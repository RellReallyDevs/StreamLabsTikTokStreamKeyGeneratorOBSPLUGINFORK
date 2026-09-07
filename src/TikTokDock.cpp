#include "TikTokDock.h"

#include <obs.h>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>

#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QJsonObject>

static void tiktok_dock_frontend_event(enum obs_frontend_event event, void *data)
{
	auto *dock = static_cast<TikTokDock *>(data);
	if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPED) {
		QMetaObject::invokeMethod(dock, "onFrontendStreamingStopped", Qt::QueuedConnection);
	}
}

TikTokDock::TikTokDock(QWidget *parent) : QWidget(parent)
{
	m_client = new StreamClient(this);

	m_searchDebounce = new QTimer(this);
	m_searchDebounce->setSingleShot(true);
	m_searchDebounce->setInterval(300);

	buildUi();

	connect(m_client, &StreamClient::categoriesReady, this, &TikTokDock::onCategoriesReady);
	connect(m_client, &StreamClient::streamStarted, this, &TikTokDock::onStreamStarted);
	connect(m_client, &StreamClient::streamStartFailed, this, &TikTokDock::onStreamStartFailed);
	connect(m_client, &StreamClient::streamEnded, this, &TikTokDock::onStreamEnded);
	connect(m_client, &StreamClient::accountInfoReady, this, &TikTokDock::onAccountInfoReady);
	connect(m_client, &StreamClient::requestFailed, this, &TikTokDock::onRequestFailed);
	connect(m_searchDebounce, &QTimer::timeout, this, &TikTokDock::onCategorySearchTimeout);

	obs_frontend_add_event_callback(tiktok_dock_frontend_event, this);

	loadSavedToken();
}

TikTokDock::~TikTokDock()
{
	obs_frontend_remove_event_callback(tiktok_dock_frontend_event, this);
}

void TikTokDock::buildUi()
{
	auto *root = new QVBoxLayout(this);

	// --- Account / token -----------------------------------------------
	auto *accountBox = new QGroupBox(tr("Streamlabs Account"), this);
	auto *accountLayout = new QVBoxLayout(accountBox);

	auto *tokenRow = new QHBoxLayout();
	m_tokenEdit = new QLineEdit(accountBox);
	m_tokenEdit->setPlaceholderText(tr("Paste your Streamlabs API token"));
	m_tokenEdit->setEchoMode(QLineEdit::Password);
	m_saveTokenBtn = new QPushButton(tr("Save"), accountBox);
	tokenRow->addWidget(m_tokenEdit);
	tokenRow->addWidget(m_saveTokenBtn);
	accountLayout->addLayout(tokenRow);

	m_accountLabel = new QLabel(tr("Not connected"), accountBox);
	accountLayout->addWidget(m_accountLabel);

	root->addWidget(accountBox);

	// --- Stream setup -----------------------------------------------
	auto *setupBox = new QGroupBox(tr("Stream Setup"), this);
	auto *form = new QFormLayout(setupBox);

	m_titleEdit = new QLineEdit(setupBox);
	m_titleEdit->setPlaceholderText(tr("Stream title"));
	form->addRow(tr("Title"), m_titleEdit);

	m_categoryCombo = new QComboBox(setupBox);
	m_categoryCombo->setEditable(true);
	m_categoryCombo->setInsertPolicy(QComboBox::NoInsert);
	form->addRow(tr("Game / Category"), m_categoryCombo);

	m_matureCheck = new QCheckBox(tr("Mature audience (18+)"), setupBox);
	form->addRow(QString(), m_matureCheck);

	root->addWidget(setupBox);

	// --- Controls -----------------------------------------------
	auto *controlsRow = new QHBoxLayout();
	m_goLiveBtn = new QPushButton(tr("Go Live"), this);
	m_endLiveBtn = new QPushButton(tr("End Live"), this);
	m_endLiveBtn->setEnabled(false);
	controlsRow->addWidget(m_goLiveBtn);
	controlsRow->addWidget(m_endLiveBtn);
	root->addLayout(controlsRow);

	m_statusLabel = new QLabel(tr("Idle"), this);
	root->addWidget(m_statusLabel);
	root->addStretch();

	connect(m_saveTokenBtn, &QPushButton::clicked, this, &TikTokDock::onSaveTokenClicked);
	connect(m_categoryCombo->lineEdit(), &QLineEdit::textEdited, this, &TikTokDock::onCategoryTextEdited);
	connect(m_goLiveBtn, &QPushButton::clicked, this, &TikTokDock::onGoLiveClicked);
	connect(m_endLiveBtn, &QPushButton::clicked, this, &TikTokDock::onEndLiveClicked);
}

void TikTokDock::onSaveTokenClicked()
{
	const QString token = m_tokenEdit->text().trimmed();
	saveToken(token);
	m_client->setToken(token);
	m_accountLabel->setText(tr("Checking..."));
	m_client->fetchAccountInfo();
}

void TikTokDock::onCategoryTextEdited(const QString &text)
{
	m_pendingSearchText = text;
	m_searchDebounce->start();
}

void TikTokDock::onCategorySearchTimeout()
{
	m_client->searchCategories(m_pendingSearchText);
}

void TikTokDock::onCategoriesReady(const QList<TikTokCategory> &categories)
{
	const QString currentText = m_categoryCombo->currentText();
	m_categoryCombo->blockSignals(true);
	m_categoryCombo->clear();
	for (const TikTokCategory &cat : categories)
		m_categoryCombo->addItem(cat.fullName, cat.gameMaskId);
	m_categoryCombo->setCurrentText(currentText);
	m_categoryCombo->blockSignals(false);
}

void TikTokDock::onGoLiveClicked()
{
	if (!m_client->hasToken()) {
		m_statusLabel->setText(tr("Save your Streamlabs token first"));
		return;
	}

	const QString title = m_titleEdit->text().trimmed();
	const QString categoryId = m_categoryCombo->currentData().toString();
	const QString audience = m_matureCheck->isChecked() ? QStringLiteral("1") : QStringLiteral("0");

	if (title.isEmpty()) {
		m_statusLabel->setText(tr("Enter a stream title first"));
		return;
	}

	m_goLiveBtn->setEnabled(false);
	m_statusLabel->setText(tr("Starting TikTok LIVE room..."));
	m_client->startStream(title, categoryId, audience);
}

void TikTokDock::onEndLiveClicked()
{
	m_endLiveBtn->setEnabled(false);
	m_statusLabel->setText(tr("Ending stream..."));
	obs_frontend_streaming_stop();
	m_client->endStream();
}

void TikTokDock::onStreamStarted(const QString &rtmp, const QString &key)
{
	applyStreamSettings(rtmp, key);
}

void TikTokDock::onStreamStartFailed(const QString &error)
{
	m_goLiveBtn->setEnabled(true);
	m_statusLabel->setText(tr("Failed to start TikTok LIVE: %1").arg(error));
	blog(LOG_WARNING, "[tiktok-live-dock] start failed: %s", error.toUtf8().constData());
}

void TikTokDock::onStreamEnded(bool success)
{
	setLiveState(false);
	m_statusLabel->setText(success ? tr("Stream ended") : tr("Stream ended (TikTok reported an issue)"));
}

void TikTokDock::onAccountInfoReady(const QJsonObject &info)
{
	if (info.isEmpty()) {
		m_accountLabel->setText(tr("Not connected"));
		return;
	}
	m_accountLabel->setText(tr("Connected"));
}

void TikTokDock::onRequestFailed(const QString &error)
{
	blog(LOG_WARNING, "[tiktok-live-dock] request failed: %s", error.toUtf8().constData());
}

void TikTokDock::onFrontendStreamingStopped()
{
	// Keeps the TikTok room in sync if the user hits OBS's own "Stop
	// Streaming" button instead of this dock's "End Live" button.
	if (m_liveViaThisDock) {
		m_client->endStream();
		setLiveState(false);
		m_statusLabel->setText(tr("Stream ended"));
	}
}

void TikTokDock::applyStreamSettings(const QString &rtmp, const QString &key)
{
	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "server", rtmp.toUtf8().constData());
	obs_data_set_string(settings, "key", key.toUtf8().constData());

	obs_service_t *service = obs_service_create("rtmp_custom", "TikTokLiveService", settings, nullptr);
	obs_data_release(settings);

	if (!service) {
		m_goLiveBtn->setEnabled(true);
		m_statusLabel->setText(tr("Failed to create OBS streaming service"));
		return;
	}

	obs_frontend_set_streaming_service(service);
	obs_service_release(service);
	obs_frontend_save_streaming_service();

	obs_frontend_streaming_start();

	setLiveState(true);
	m_statusLabel->setText(tr("Live on TikTok"));
}

void TikTokDock::setLiveState(bool live)
{
	m_liveViaThisDock = live;
	m_goLiveBtn->setEnabled(!live);
	m_endLiveBtn->setEnabled(live);
}

void TikTokDock::loadSavedToken()
{
	char *dir = obs_module_config_path("");
	if (dir) {
		os_mkdirs(dir);
		bfree(dir);
	}

	char *path = obs_module_config_path("token.json");
	if (!path)
		return;

	obs_data_t *data = obs_data_create_from_json_file(path);
	bfree(path);
	if (!data)
		return;

	const QString token = QString::fromUtf8(obs_data_get_string(data, "token"));
	obs_data_release(data);

	if (!token.isEmpty()) {
		m_tokenEdit->setText(token);
		m_client->setToken(token);
		m_client->fetchAccountInfo();
	}
}

void TikTokDock::saveToken(const QString &token)
{
	char *dir = obs_module_config_path("");
	if (dir) {
		os_mkdirs(dir);
		bfree(dir);
	}

	char *path = obs_module_config_path("token.json");
	if (!path)
		return;

	obs_data_t *data = obs_data_create();
	obs_data_set_string(data, "token", token.toUtf8().constData());
	obs_data_save_json(data, path);
	obs_data_release(data);
	bfree(path);
}

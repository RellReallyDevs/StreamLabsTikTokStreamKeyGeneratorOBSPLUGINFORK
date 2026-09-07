#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

#include "StreamClient.h"

// Single "all-in-one" panel docked inside OBS:
//  - Streamlabs API token entry (paste-once, persisted to plugin config)
//  - Stream title
//  - Game / category picker with live autocomplete against TikTok's category list
//  - Mature/audience toggle
//  - Go Live / End Live buttons that both provision the TikTok room AND
//    drive OBS's own streaming output directly (no manual copy/paste of
//    server + key into Settings -> Stream).
class TikTokDock : public QWidget {
	Q_OBJECT
public:
	explicit TikTokDock(QWidget *parent = nullptr);
	~TikTokDock() override;

private slots:
	void onSaveTokenClicked();
	void onCategoryTextEdited(const QString &text);
	void onCategorySearchTimeout();
	void onCategoriesReady(const QList<TikTokCategory> &categories);
	void onGoLiveClicked();
	void onEndLiveClicked();
	void onStreamStarted(const QString &rtmp, const QString &key);
	void onStreamStartFailed(const QString &error);
	void onStreamEnded(bool success);
	void onAccountInfoReady(const QJsonObject &info);
	void onRequestFailed(const QString &error);
	void onFrontendStreamingStopped();

private:
	void buildUi();
	void applyStreamSettings(const QString &rtmp, const QString &key);
	void loadSavedToken();
	void saveToken(const QString &token);
	void setLiveState(bool live);

	StreamClient *m_client = nullptr;

	QLineEdit *m_tokenEdit = nullptr;
	QPushButton *m_saveTokenBtn = nullptr;
	QLabel *m_accountLabel = nullptr;

	QLineEdit *m_titleEdit = nullptr;
	QComboBox *m_categoryCombo = nullptr;
	QCheckBox *m_matureCheck = nullptr;

	QPushButton *m_goLiveBtn = nullptr;
	QPushButton *m_endLiveBtn = nullptr;
	QLabel *m_statusLabel = nullptr;

	QTimer *m_searchDebounce = nullptr;
	QString m_pendingSearchText;

	bool m_liveViaThisDock = false;
};

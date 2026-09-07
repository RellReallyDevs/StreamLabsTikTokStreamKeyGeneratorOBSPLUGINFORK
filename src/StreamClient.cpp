#include "StreamClient.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QUrlQuery>
#include <QUrl>

// Same base path used by Stream.py: https://streamlabs.com/api/v5/slobs/tiktok
const QString StreamClient::kBaseUrl = QStringLiteral("https://streamlabs.com/api/v5/slobs/tiktok");

// Matches the User-Agent Stream.py sends so Streamlabs treats requests as
// coming from a real Streamlabs Desktop client.
const QString StreamClient::kUserAgent = QStringLiteral(
	"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
	"StreamlabsDesktop/1.17.0 Chrome/122.0.6261.156 Electron/29.3.1 Safari/537.36");

StreamClient::StreamClient(QObject *parent) : QObject(parent) {}

void StreamClient::setToken(const QString &token)
{
	m_token = token;
}

bool StreamClient::hasToken() const
{
	return !m_token.isEmpty();
}

QNetworkRequest StreamClient::buildRequest(const QString &url) const
{
	QNetworkRequest req{QUrl(url)};
	req.setRawHeader("User-Agent", kUserAgent.toUtf8());
	req.setRawHeader("Authorization", QByteArray("Bearer ") + m_token.toUtf8());
	return req;
}

void StreamClient::searchCategories(const QString &game)
{
	if (game.isEmpty()) {
		emit categoriesReady({});
		return;
	}

	// The Streamlabs API returns HTTP 500 for category strings longer than
	// 25 characters, same limit Stream.py enforces.
	const QString trimmed = game.left(25);

	QUrl url(kBaseUrl + "/info");
	QUrlQuery query;
	query.addQueryItem("category", trimmed);
	url.setQuery(query);

	QNetworkReply *reply = m_nam.get(buildRequest(url.toString()));
	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		reply->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			emit requestFailed(reply->errorString());
			return;
		}

		const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
		QList<TikTokCategory> result;
		for (const QJsonValue &item : obj.value("categories").toArray()) {
			const QJsonObject cat = item.toObject();
			result.append({cat.value("full_name").toString(), cat.value("game_mask_id").toString()});
		}
		// Mirrors the Python code always appending a manual "Other" fallback.
		result.append({QStringLiteral("Other"), QString()});

		emit categoriesReady(result);
	});
}

void StreamClient::startStream(const QString &title, const QString &categoryId, const QString &audienceType)
{
	auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

	auto makePart = [](const QString &name, const QString &value) {
		QHttpPart part;
		part.setHeader(QNetworkRequest::ContentDispositionHeader,
			       QVariant(QStringLiteral("form-data; name=\"%1\"").arg(name)));
		part.setBody(value.toUtf8());
		return part;
	};

	multiPart->append(makePart(QStringLiteral("title"), title));
	multiPart->append(makePart(QStringLiteral("device_platform"), QStringLiteral("win32")));
	multiPart->append(makePart(QStringLiteral("category"), categoryId));
	multiPart->append(makePart(QStringLiteral("audience_type"), audienceType));

	QNetworkRequest req = buildRequest(kBaseUrl + "/stream/start");
	QNetworkReply *reply = m_nam.post(req, multiPart);
	multiPart->setParent(reply);

	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		reply->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			emit streamStartFailed(reply->errorString());
			return;
		}

		const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
		if (!obj.contains("id") || !obj.contains("rtmp") || !obj.contains("key")) {
			emit streamStartFailed(QString::fromUtf8(
				QJsonDocument(obj).toJson(QJsonDocument::Compact)));
			return;
		}

		m_streamId = obj.value("id").isString() ? obj.value("id").toString()
							 : QString::number(obj.value("id").toVariant().toLongLong());

		emit streamStarted(obj.value("rtmp").toString(), obj.value("key").toString());
	});
}

void StreamClient::endStream()
{
	if (m_streamId.isEmpty()) {
		emit streamEnded(false);
		return;
	}

	QNetworkRequest req = buildRequest(kBaseUrl + "/stream/" + m_streamId + "/end");
	QNetworkReply *reply = m_nam.post(req, QByteArray());
	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		reply->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			emit requestFailed(reply->errorString());
			emit streamEnded(false);
			return;
		}

		const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
		emit streamEnded(obj.value("success").toBool());
	});
}

void StreamClient::fetchAccountInfo()
{
	QNetworkReply *reply = m_nam.get(buildRequest(kBaseUrl + "/info"));
	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		reply->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			emit requestFailed(reply->errorString());
			return;
		}
		emit accountInfoReady(QJsonDocument::fromJson(reply->readAll()).object());
	});
}

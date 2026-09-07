#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>
#include <QList>
#include <QJsonObject>

struct TikTokCategory {
	QString fullName;
	QString gameMaskId;
};

// C++ port of the Python Stream class (Stream.py) from the standalone
// StreamLabsTikTokStreamKeyGenerator app. Talks to the same licensed
// Streamlabs "slobs" TikTok LIVE endpoints, so no reverse-engineered
// TikTok signing / FFmpeg proxy is required.
class StreamClient : public QObject {
	Q_OBJECT
public:
	explicit StreamClient(QObject *parent = nullptr);

	void setToken(const QString &token);
	bool hasToken() const;

	void searchCategories(const QString &game);
	void startStream(const QString &title, const QString &categoryId, const QString &audienceType = "0");
	void endStream();
	void fetchAccountInfo();

signals:
	void categoriesReady(const QList<TikTokCategory> &categories);
	void streamStarted(const QString &rtmp, const QString &key);
	void streamStartFailed(const QString &error);
	void streamEnded(bool success);
	void accountInfoReady(const QJsonObject &info);
	void requestFailed(const QString &error);

private:
	QNetworkRequest buildRequest(const QString &url) const;

	QNetworkAccessManager m_nam;
	QString m_token;
	QString m_streamId;

	static const QString kBaseUrl;
	static const QString kUserAgent;
};

#include "islamicbridge.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

IslamicBridge::IslamicBridge(QObject *parent)
    : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
}

void IslamicBridge::setEndpoint(const QString &url)
{
    m_endpoint = url;
}

QNetworkRequest IslamicBridge::makeRequest(const QString &path) const
{
    QNetworkRequest request(QUrl(m_endpoint + path));
    request.setHeader(
        QNetworkRequest::ContentTypeHeader, "application/json");
    return request;
}

void IslamicBridge::testConnection()
{
    QNetworkRequest request(QUrl(m_endpoint + "/health"));
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished,
            this, &IslamicBridge::onConnectionReply);
}

void IslamicBridge::onConnectionReply()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        m_connected = true;
        emit connectionTested(
            true, "Connected to Islamic knowledge service");
    } else {
        m_connected = false;
        emit connectionTested(false, reply->errorString());
    }
    reply->deleteLater();
}

void IslamicBridge::validateHadith(const QString &text)
{
    QJsonObject body;
    body["text"] = text;
    body["action"] = "validate_hadith";

    QNetworkReply *reply = m_networkManager->post(
        makeRequest("/api/hadith/validate"),
        QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished,
            this, &IslamicBridge::onHadithReply);
}

void IslamicBridge::onHadithReply()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;

    HadithValidation result;

    if (reply->error() != QNetworkReply::NoError) {
        result.error = reply->errorString();
        emit hadithValidated(result);
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    result.hadithText = obj["hadith_text"].toString();
    result.grading = obj["grading"].toString();
    result.source = obj["source"].toString();
    result.gradingAuthority =
        obj["grading_authority"].toString();
    result.isValid = obj["is_valid"].toBool();
    result.error = obj["error"].toString();

    emit hadithValidated(result);
    reply->deleteLater();
}

void IslamicBridge::validateQuranCitation(
    const QString &surah, int ayah)
{
    QJsonObject body;
    body["surah"] = surah;
    body["ayah"] = ayah;
    body["action"] = "validate_quran";

    QNetworkReply *reply = m_networkManager->post(
        makeRequest("/api/quran/validate"),
        QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished,
            this, &IslamicBridge::onQuranReply);
}

void IslamicBridge::onQuranReply()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;

    QuranValidation result;

    if (reply->error() != QNetworkReply::NoError) {
        result.error = reply->errorString();
        emit quranValidated(result);
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    result.surah = obj["surah"].toString();
    result.ayah = obj["ayah"].toInt();
    result.arabicText = obj["arabic_text"].toString();
    result.translation = obj["translation"].toString();
    result.isValid = obj["is_valid"].toBool();
    result.error = obj["error"].toString();

    emit quranValidated(result);
    reply->deleteLater();
}

void IslamicBridge::lookupScholar(const QString &name)
{
    QJsonObject body;
    body["name"] = name;
    body["action"] = "lookup_scholar";

    QNetworkReply *reply = m_networkManager->post(
        makeRequest("/api/scholar/lookup"),
        QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished,
            this, &IslamicBridge::onScholarReply);
}

void IslamicBridge::onScholarReply()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    emit scholarResult(obj["name"].toString(),
                       obj["info"].toString());
    reply->deleteLater();
}

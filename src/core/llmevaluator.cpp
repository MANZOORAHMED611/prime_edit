#include "llmevaluator.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QFileInfo>

LLMEvaluator::LLMEvaluator(QObject *parent) : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
}

void LLMEvaluator::evaluate(const QString &text, const EvalEndpoint &endpoint)
{
    cancel();
    m_accumulatedResponse.clear();
    m_currentEndpoint = endpoint;

    QNetworkRequest request(QUrl(endpoint.url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (!endpoint.apiKey.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + endpoint.apiKey).toUtf8());
    }

    QJsonObject body;

    if (endpoint.type == EvalEndpoint::Ollama) {
        body["model"] = endpoint.model;
        body["prompt"] = endpoint.systemPrompt + "\n\n" + text;
        body["stream"] = false;
    } else {
        QJsonArray messages;
        QJsonObject sysMsg;
        sysMsg["role"] = "system";
        sysMsg["content"] = endpoint.systemPrompt;
        messages.append(sysMsg);

        QJsonObject userMsg;
        userMsg["role"] = "user";
        userMsg["content"] = text;
        messages.append(userMsg);

        body["model"] = endpoint.model;
        body["messages"] = messages;
    }

    m_currentReply = m_networkManager->post(request, QJsonDocument(body).toJson());
    connect(m_currentReply, &QNetworkReply::finished, this, &LLMEvaluator::onReplyFinished);
    connect(m_currentReply, &QNetworkReply::readyRead, this, &LLMEvaluator::onReadyRead);
}

void LLMEvaluator::cancel()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

void LLMEvaluator::onReadyRead()
{
    if (!m_currentReply) return;
    QByteArray data = m_currentReply->readAll();
    m_accumulatedResponse += QString::fromUtf8(data);
}

void LLMEvaluator::onReplyFinished()
{
    if (!m_currentReply) return;

    if (m_currentReply->error() != QNetworkReply::NoError) {
        emit errorOccurred(m_currentReply->errorString());
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }

    QByteArray responseData = m_accumulatedResponse.toUtf8();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);

    QString result;
    if (m_currentEndpoint.type == EvalEndpoint::Ollama) {
        result = doc.object()["response"].toString();
    } else {
        QJsonArray choices = doc.object()["choices"].toArray();
        if (!choices.isEmpty()) {
            result = choices[0].toObject()["message"].toObject()["content"].toString();
        }
    }

    if (result.isEmpty()) {
        result = m_accumulatedResponse;
    }

    emit resultReady(result.trimmed());

    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

EvalEndpoint LLMEvaluator::loadEndpoint()
{
    EvalEndpoint endpoint;
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                         + "/eval-endpoint.json";
    QFile file(configPath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
        endpoint.name = obj.value("name").toString("Ollama");
        endpoint.url = obj.value("url").toString("http://localhost:11434/api/generate");
        endpoint.model = obj.value("model").toString("llama3");
        endpoint.apiKey = obj.value("apiKey").toString();
        endpoint.systemPrompt = obj.value("systemPrompt").toString(endpoint.systemPrompt);
        endpoint.type = (obj.value("type").toString() == "openai")
                            ? EvalEndpoint::OpenAICompatible
                            : EvalEndpoint::Ollama;
    }
    return endpoint;
}

void LLMEvaluator::saveEndpoint(const EvalEndpoint &endpoint)
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                         + "/eval-endpoint.json";
    QDir().mkpath(QFileInfo(configPath).absolutePath());
    QFile file(configPath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject obj;
        obj["name"] = endpoint.name;
        obj["url"] = endpoint.url;
        obj["model"] = endpoint.model;
        obj["apiKey"] = endpoint.apiKey;
        obj["systemPrompt"] = endpoint.systemPrompt;
        obj["type"] = (endpoint.type == EvalEndpoint::OpenAICompatible) ? "openai" : "ollama";
        file.write(QJsonDocument(obj).toJson());
    }
}

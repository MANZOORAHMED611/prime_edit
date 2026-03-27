#include "lspclient.h"
#include <QJsonArray>
#include <QDebug>
#include <QFileInfo>
#include <QCoreApplication>

LSPClient::LSPClient(const QString &serverCommand, const QStringList &args, QObject *parent)
    : QObject(parent)
    , m_process(nullptr)
    , m_serverCommand(serverCommand)
    , m_serverArgs(args)
    , m_nextId(1)
    , m_initialized(false)
{
}

LSPClient::~LSPClient()
{
    shutdown();
}

void LSPClient::initialize(const QString &rootPath)
{
    m_rootPath = rootPath;

    // Start LSP server process
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &LSPClient::onReadyRead);
    connect(m_process, &QProcess::errorOccurred, this, &LSPClient::onProcessError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &LSPClient::onProcessFinished);

    m_process->start(m_serverCommand, m_serverArgs);

    if (!m_process->waitForStarted(5000)) {
        emit errorOccurred("Failed to start LSP server: " + m_serverCommand);
        return;
    }

    // Send initialize request
    QJsonObject params;
    params["processId"] = QCoreApplication::applicationPid();
    params["rootPath"] = m_rootPath;
    params["rootUri"] = "file://" + m_rootPath;

    QJsonObject capabilities;
    QJsonObject textDocument;
    textDocument["synchronization"] = QJsonObject{{"dynamicRegistration", false}};
    textDocument["completion"] = QJsonObject{{"dynamicRegistration", false}};
    textDocument["hover"] = QJsonObject{{"dynamicRegistration", false}};
    textDocument["definition"] = QJsonObject{{"dynamicRegistration", false}};
    textDocument["references"] = QJsonObject{{"dynamicRegistration", false}};
    capabilities["textDocument"] = textDocument;
    params["capabilities"] = capabilities;

    sendRequest("initialize", params);
}

void LSPClient::shutdown()
{
    if (m_process && m_process->state() == QProcess::Running) {
        sendRequest("shutdown", QJsonObject());
        sendNotification("exit", QJsonObject());

        m_process->waitForFinished(2000);
        if (m_process->state() == QProcess::Running) {
            m_process->kill();
        }
    }

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }

    m_initialized = false;
}

void LSPClient::didOpen(const QString &uri, const QString &languageId, const QString &text)
{
    if (!m_initialized) return;

    QJsonObject params;
    QJsonObject textDocument;
    textDocument["uri"] = uri;
    textDocument["languageId"] = languageId;
    textDocument["version"] = 1;
    textDocument["text"] = text;
    params["textDocument"] = textDocument;

    sendNotification("textDocument/didOpen", params);
}

void LSPClient::didChange(const QString &uri, const QString &text)
{
    if (!m_initialized) return;

    QJsonObject params;
    QJsonObject textDocument;
    textDocument["uri"] = uri;
    textDocument["version"] = m_documentVersion++;
    params["textDocument"] = textDocument;

    QJsonArray contentChanges;
    QJsonObject change;
    change["text"] = text;
    contentChanges.append(change);
    params["contentChanges"] = contentChanges;

    sendNotification("textDocument/didChange", params);
}

void LSPClient::didClose(const QString &uri)
{
    if (!m_initialized) return;

    QJsonObject params;
    QJsonObject textDocument;
    textDocument["uri"] = uri;
    params["textDocument"] = textDocument;

    sendNotification("textDocument/didClose", params);
}

void LSPClient::didSave(const QString &uri)
{
    if (!m_initialized) return;

    QJsonObject params;
    QJsonObject textDocument;
    textDocument["uri"] = uri;
    params["textDocument"] = textDocument;

    sendNotification("textDocument/didSave", params);
}

void LSPClient::completion(const QString &uri, int line, int character)
{
    if (!m_initialized) return;

    QJsonObject params;
    QJsonObject textDocument;
    textDocument["uri"] = uri;
    params["textDocument"] = textDocument;

    QJsonObject position;
    position["line"] = line;
    position["character"] = character;
    params["position"] = position;

    sendRequest("textDocument/completion", params);
}

void LSPClient::hover(const QString &uri, int line, int character)
{
    if (!m_initialized) return;

    QJsonObject params;
    QJsonObject textDocument;
    textDocument["uri"] = uri;
    params["textDocument"] = textDocument;

    QJsonObject position;
    position["line"] = line;
    position["character"] = character;
    params["position"] = position;

    sendRequest("textDocument/hover", params);
}

void LSPClient::gotoDefinition(const QString &uri, int line, int character)
{
    if (!m_initialized) return;

    QJsonObject params;
    QJsonObject textDocument;
    textDocument["uri"] = uri;
    params["textDocument"] = textDocument;

    QJsonObject position;
    position["line"] = line;
    position["character"] = character;
    params["position"] = position;

    sendRequest("textDocument/definition", params);
}

void LSPClient::references(const QString &uri, int line, int character)
{
    if (!m_initialized) return;

    QJsonObject params;
    QJsonObject textDocument;
    textDocument["uri"] = uri;
    params["textDocument"] = textDocument;

    QJsonObject position;
    position["line"] = line;
    position["character"] = character;
    params["position"] = position;

    QJsonObject context;
    context["includeDeclaration"] = true;
    params["context"] = context;

    sendRequest("textDocument/references", params);
}

void LSPClient::rename(const QString &uri, int line, int character,
                       const QString &newName)
{
    if (!m_initialized) return;

    QJsonObject params;
    QJsonObject textDocument;
    textDocument["uri"] = uri;
    params["textDocument"] = textDocument;

    QJsonObject position;
    position["line"] = line;
    position["character"] = character;
    params["position"] = position;
    params["newName"] = newName;

    sendRequest("textDocument/rename", params);
}

bool LSPClient::isRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}

void LSPClient::sendRequest(const QString &method, const QJsonObject &params)
{
    int id = m_nextId++;
    m_pendingRequests[id] = method;

    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["id"] = id;
    message["method"] = method;
    message["params"] = params;

    QJsonDocument doc(message);
    QByteArray content = doc.toJson(QJsonDocument::Compact);

    QString header = QString("Content-Length: %1\r\n\r\n").arg(content.size());
    QByteArray message_data = header.toUtf8() + content;

    if (m_process && m_process->state() == QProcess::Running) {
        m_process->write(message_data);
        m_process->waitForBytesWritten();
    }
}

void LSPClient::sendNotification(const QString &method, const QJsonObject &params)
{
    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["method"] = method;
    message["params"] = params;

    QJsonDocument doc(message);
    QByteArray content = doc.toJson(QJsonDocument::Compact);

    QString header = QString("Content-Length: %1\r\n\r\n").arg(content.size());
    QByteArray message_data = header.toUtf8() + content;

    if (m_process && m_process->state() == QProcess::Running) {
        m_process->write(message_data);
        m_process->waitForBytesWritten();
    }
}

void LSPClient::onReadyRead()
{
    m_buffer.append(m_process->readAllStandardOutput());

    while (true) {
        // Parse Content-Length header
        int headerEnd = m_buffer.indexOf("\r\n\r\n");
        if (headerEnd == -1) break;

        QString headers = QString::fromUtf8(m_buffer.left(headerEnd));
        QStringList headerLines = headers.split("\r\n");

        int contentLength = 0;
        for (const QString &line : headerLines) {
            if (line.startsWith("Content-Length:")) {
                contentLength = line.mid(15).trimmed().toInt();
                break;
            }
        }

        if (contentLength == 0) {
            m_buffer.remove(0, headerEnd + 4);
            continue;
        }

        int messageStart = headerEnd + 4;
        if (m_buffer.size() < messageStart + contentLength) break;

        QByteArray messageData = m_buffer.mid(messageStart, contentLength);
        m_buffer.remove(0, messageStart + contentLength);

        QJsonObject message = parseMessage(messageData);
        if (!message.isEmpty()) {
            processMessage(message);
        }
    }
}

void LSPClient::onProcessError(QProcess::ProcessError error)
{
    QString errorStr;
    switch (error) {
    case QProcess::FailedToStart:
        errorStr = "LSP server failed to start";
        break;
    case QProcess::Crashed:
        errorStr = "LSP server crashed";
        break;
    default:
        errorStr = "LSP server error";
        break;
    }

    emit errorOccurred(errorStr);
}

void LSPClient::onProcessFinished(int exitCode)
{
    Q_UNUSED(exitCode);
    m_initialized = false;
    emit shutdownComplete();
}

QJsonObject LSPClient::parseMessage(const QByteArray &data)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "LSP JSON parse error:" << error.errorString();
        return QJsonObject();
    }

    return doc.object();
}

void LSPClient::processMessage(const QJsonObject &message)
{
    if (message.contains("id")) {
        // This is a response
        handleResponse(message);
    } else if (message.contains("method")) {
        // This is a notification
        QString method = message["method"].toString();
        QJsonObject params = message["params"].toObject();
        handleNotification(method, params);
    }
}

void LSPClient::handleResponse(const QJsonObject &response)
{
    int id = response["id"].toInt();
    QString method = m_pendingRequests.take(id);

    if (method == "initialize") {
        m_initialized = true;
        sendNotification("initialized", QJsonObject());
        emit initialized();
    } else if (method == "textDocument/completion") {
        QVector<CompletionItem> items;
        QJsonValue result = response["result"];

        if (result.isArray()) {
            QJsonArray array = result.toArray();
            for (const QJsonValue &val : array) {
                QJsonObject obj = val.toObject();
                CompletionItem item;
                item.label = obj["label"].toString();
                item.detail = obj["detail"].toString();
                item.documentation = obj["documentation"].toString();
                item.kind = obj["kind"].toInt();
                items.append(item);
            }
        } else if (result.isObject()) {
            QJsonObject obj = result.toObject();
            QJsonArray itemsArray = obj["items"].toArray();
            for (const QJsonValue &val : itemsArray) {
                QJsonObject itemObj = val.toObject();
                CompletionItem item;
                item.label = itemObj["label"].toString();
                item.detail = itemObj["detail"].toString();
                item.documentation = itemObj["documentation"].toString();
                item.kind = itemObj["kind"].toInt();
                items.append(item);
            }
        }

        emit completionResult(items);
    } else if (method == "textDocument/hover") {
        QJsonObject result = response["result"].toObject();
        QJsonValue contents = result["contents"];

        QString hoverText;
        if (contents.isString()) {
            hoverText = contents.toString();
        } else if (contents.isObject()) {
            hoverText = contents.toObject()["value"].toString();
        }

        emit hoverResult(hoverText);
    } else if (method == "textDocument/definition") {
        QJsonValue result = response["result"];
        Location location;
        if (result.isArray()) {
            QJsonArray arr = result.toArray();
            if (!arr.isEmpty()) {
                QJsonObject loc = arr.first().toObject();
                location.uri = loc["uri"].toString();
                QJsonObject range = loc["range"].toObject();
                QJsonObject start = range["start"].toObject();
                location.line = start["line"].toInt();
                location.character = start["character"].toInt();
            }
        } else if (result.isObject()) {
            QJsonObject loc = result.toObject();
            location.uri = loc["uri"].toString();
            QJsonObject range = loc["range"].toObject();
            QJsonObject start = range["start"].toObject();
            location.line = start["line"].toInt();
            location.character = start["character"].toInt();
        }
        if (!location.uri.isEmpty()) {
            emit definitionResult(location);
        }
    } else if (method == "textDocument/references") {
        QVector<Location> locations;
        QJsonValue result = response["result"];
        if (result.isArray()) {
            QJsonArray arr = result.toArray();
            for (const QJsonValue &val : arr) {
                QJsonObject loc = val.toObject();
                Location l;
                l.uri = loc["uri"].toString();
                QJsonObject range = loc["range"].toObject();
                QJsonObject start = range["start"].toObject();
                l.line = start["line"].toInt();
                l.character = start["character"].toInt();
                locations.append(l);
            }
        }
        emit referencesResult(locations);
    } else if (method == "textDocument/rename") {
        QJsonObject result = response["result"].toObject();
        emit renameResult(result);
    }
}

void LSPClient::handleNotification(const QString &method, const QJsonObject &params)
{
    if (method == "textDocument/publishDiagnostics") {
        QString uri = params["uri"].toString();
        QJsonArray diagnosticsArray = params["diagnostics"].toArray();

        QVector<Diagnostic> diagnostics;
        for (const QJsonValue &val : diagnosticsArray) {
            QJsonObject obj = val.toObject();
            Diagnostic diag;
            diag.message = obj["message"].toString();
            diag.severity = obj["severity"].toInt();

            QJsonObject range = obj["range"].toObject();
            QJsonObject start = range["start"].toObject();
            QJsonObject end = range["end"].toObject();

            diag.line = start["line"].toInt();
            diag.character = start["character"].toInt();
            diag.endLine = end["line"].toInt();
            diag.endCharacter = end["character"].toInt();

            diagnostics.append(diag);
        }

        emit diagnosticsPublished(uri, diagnostics);
    }
}

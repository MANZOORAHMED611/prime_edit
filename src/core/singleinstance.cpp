#include "singleinstance.h"

#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QStandardPaths>

namespace {

// A request is one short JSON line; anything larger is not ours.
constexpr qint64 kMaxRequestBytes = 1024 * 1024;

QString appDataDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QByteArray requestLine(const QStringList &files, int gotoLine)
{
    QJsonObject message;
    message["v"] = 1;
    message["files"] = QJsonArray::fromStringList(files);
    message["goto"] = gotoLine > 0 ? QJsonValue(gotoLine) : QJsonValue(QJsonValue::Null);
    return QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n';
}

// Reads one newline-terminated line within the remaining time budget.
QByteArray readLine(QLocalSocket &socket, int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    while (!socket.canReadLine()) {
        const qint64 remaining = timeoutMs - timer.elapsed();
        if (remaining <= 0 || !socket.waitForReadyRead(int(remaining))) {
            return QByteArray();
        }
    }
    return socket.readLine();
}

} // namespace

SingleInstance::SingleInstance(QObject *parent)
    : QObject(parent)
{
}

SingleInstance::~SingleInstance() = default;

QString SingleInstance::serverName()
{
    // Per user and per profile: a scratch HOME can never reach the live
    // editor, and another user cannot claim the name first.
    const QString runtimeDir = qEnvironmentVariable("XDG_RUNTIME_DIR");
    if (runtimeDir.isEmpty()) {
        return appDataDir() + QStringLiteral("/instance.sock");
    }
    const QByteArray hash =
        QCryptographicHash::hash(appDataDir().toUtf8(), QCryptographicHash::Sha1).toHex();
    return runtimeDir + QStringLiteral("/prime-edit-") + QString::fromLatin1(hash.left(16))
           + QStringLiteral(".sock");
}

QString SingleInstance::lockPath()
{
    return appDataDir() + QStringLiteral("/instance.lock");
}

bool SingleInstance::acquire()
{
    QDir().mkpath(appDataDir());
    m_lock = std::make_unique<QLockFile>(lockPath());
    // Default 30 s stale time. Checked on Qt 6.2.4: a live holder is never
    // treated as stale, and a SIGKILLed holder's lock is recovered.
    return m_lock->tryLock(0);
}

bool SingleInstance::listen()
{
    const QString name = serverName();
    QLocalServer::removeServer(name);
    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    connect(m_server, &QLocalServer::newConnection, this, &SingleInstance::onNewConnection);
    return m_server->listen(name);
}

void SingleInstance::onNewConnection()
{
    while (QLocalSocket *socket = m_server->nextPendingConnection()) {
        connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
            handleRequest(socket);
        });
    }
}

void SingleInstance::handleRequest(QLocalSocket *socket)
{
    if (!socket->canReadLine()) {
        if (socket->bytesAvailable() > kMaxRequestBytes) {
            socket->abort();
        }
        return;
    }
    const QJsonObject message = QJsonDocument::fromJson(socket->readLine()).object();
    if (message["v"].toInt() != 1) {
        socket->disconnectFromServer();
        return;
    }
    QStringList files;
    for (const QJsonValue &value : message["files"].toArray()) {
        if (QDir::isAbsolutePath(value.toString())) {
            files.append(value.toString());
        }
    }
    // Receivers connect with Qt::QueuedConnection, so the opens are queued and
    // the ack is not held up by a dialog an open might show.
    emit openRequested(files, message["goto"].toInt(0));
    socket->write("{\"ack\":true}\n");
    socket->flush();
    socket->disconnectFromServer();
}

bool SingleInstance::forward(const QStringList &absoluteFiles, int gotoLine,
                             int connectMs, int ackMs)
{
    QLocalSocket socket;
    socket.connectToServer(serverName());
    if (!socket.waitForConnected(connectMs)) {
        return false;
    }
    QElapsedTimer timer;
    timer.start();
    socket.write(requestLine(absoluteFiles, gotoLine));
    if (!socket.waitForBytesWritten(ackMs)) {
        return false;
    }
    const int remaining = ackMs - int(timer.elapsed());
    const QJsonObject reply = QJsonDocument::fromJson(readLine(socket, remaining)).object();
    return reply["ack"].toBool();
}

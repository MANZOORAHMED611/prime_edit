#include "remoteconnection.h"
#include <QRegularExpression>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

RemoteConnection::RemoteConnection(QObject *parent)
    : QObject(parent)
    , m_status(Disconnected)
    , m_type(SFTP)
    , m_port(22)
    , m_process(nullptr)
{
}

RemoteConnection::~RemoteConnection()
{
    disconnect();
}

void RemoteConnection::connectToHost(const QString &host, int port, const QString &username,
                                    const QString &password, ConnectionType type)
{
    if (m_status == Connected || m_status == Connecting) {
        return;
    }

    m_host = host;
    m_port = port;
    m_username = username;
    m_password = password;
    m_type = type;

    setStatus(Connecting);

    // Test connection with a simple command
    QStringList args;
    args << "-o" << "StrictHostKeyChecking=no"
         << "-o" << "UserKnownHostsFile=/dev/null"
         << "-p" << QString::number(port)
         << QString("%1@%2").arg(username, host)
         << "echo" << "connected";

    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
        if (exitCode == 0 && status == QProcess::NormalExit) {
            setStatus(Connected);
            emit connected();
        } else {
            m_errorString = m_process->errorString();
            setStatus(Error);
            emit connectionError(m_errorString);
        }
    });

    m_process->start("ssh", args);
}

void RemoteConnection::disconnect()
{
    if (m_process) {
        m_process->kill();
        m_process->deleteLater();
        m_process = nullptr;
    }

    setStatus(Disconnected);
    emit disconnected();
}

void RemoteConnection::setStatus(ConnectionStatus status)
{
    if (m_status != status) {
        m_status = status;
    }
}

void RemoteConnection::downloadFile(const QString &remotePath, const QString &localPath)
{
    if (m_status != Connected) {
        emit operationError("download", "Not connected");
        return;
    }

    m_pendingDownloads[remotePath] = localPath;
    m_currentOperation = "download:" + remotePath;

    QStringList args;
    if (m_type == SFTP || m_type == SCP) {
        args << "-P" << QString::number(m_port)
             << "-o" << "StrictHostKeyChecking=no"
             << "-o" << "UserKnownHostsFile=/dev/null"
             << QString("%1@%2:%3").arg(m_username, m_host, remotePath)
             << localPath;

        QProcess *proc = new QProcess(this);
        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, remotePath, localPath, proc](int exitCode, QProcess::ExitStatus status) {
            if (exitCode == 0 && status == QProcess::NormalExit) {
                emit downloadComplete(remotePath, localPath);
            } else {
                emit operationError("download", proc->errorString());
            }
            m_pendingDownloads.remove(remotePath);
            proc->deleteLater();
        });

        proc->start("scp", args);
    }
}

void RemoteConnection::uploadFile(const QString &localPath, const QString &remotePath)
{
    if (m_status != Connected) {
        emit operationError("upload", "Not connected");
        return;
    }

    m_pendingUploads[localPath] = remotePath;
    m_currentOperation = "upload:" + remotePath;

    QStringList args;
    if (m_type == SFTP || m_type == SCP) {
        args << "-P" << QString::number(m_port)
             << "-o" << "StrictHostKeyChecking=no"
             << "-o" << "UserKnownHostsFile=/dev/null"
             << localPath
             << QString("%1@%2:%3").arg(m_username, m_host, remotePath);

        QProcess *proc = new QProcess(this);
        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, remotePath, localPath, proc](int exitCode, QProcess::ExitStatus status) {
            if (exitCode == 0 && status == QProcess::NormalExit) {
                emit uploadComplete(remotePath);
            } else {
                emit operationError("upload", proc->errorString());
            }
            m_pendingUploads.remove(localPath);
            proc->deleteLater();
        });

        proc->start("scp", args);
    }
}

void RemoteConnection::listDirectory(const QString &path)
{
    if (m_status != Connected) {
        emit operationError("list", "Not connected");
        return;
    }

    QStringList args;
    args << "-p" << QString::number(m_port)
         << "-o" << "StrictHostKeyChecking=no"
         << "-o" << "UserKnownHostsFile=/dev/null"
         << QString("%1@%2").arg(m_username, m_host)
         << "ls" << "-la" << path;

    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc](int exitCode, QProcess::ExitStatus status) {
        if (exitCode == 0 && status == QProcess::NormalExit) {
            QString output = proc->readAllStandardOutput();
            QVector<RemoteFileInfo> files = parseLsOutput(output);
            emit directoryListed(files);
        } else {
            emit operationError("list", proc->errorString());
        }
        proc->deleteLater();
    });

    proc->start("ssh", args);
}

void RemoteConnection::deleteFile(const QString &remotePath)
{
    if (m_status != Connected) {
        emit operationError("delete", "Not connected");
        return;
    }

    QStringList args;
    args << "-p" << QString::number(m_port)
         << "-o" << "StrictHostKeyChecking=no"
         << "-o" << "UserKnownHostsFile=/dev/null"
         << QString("%1@%2").arg(m_username, m_host)
         << "rm" << remotePath;

    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc](int exitCode, QProcess::ExitStatus status) {
        if (exitCode != 0 || status != QProcess::NormalExit) {
            emit operationError("delete", proc->errorString());
        }
        proc->deleteLater();
    });

    proc->start("ssh", args);
}

void RemoteConnection::createDirectory(const QString &remotePath)
{
    if (m_status != Connected) {
        emit operationError("mkdir", "Not connected");
        return;
    }

    QStringList args;
    args << "-p" << QString::number(m_port)
         << "-o" << "StrictHostKeyChecking=no"
         << "-o" << "UserKnownHostsFile=/dev/null"
         << QString("%1@%2").arg(m_username, m_host)
         << "mkdir" << "-p" << remotePath;

    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc](int exitCode, QProcess::ExitStatus status) {
        if (exitCode != 0 || status != QProcess::NormalExit) {
            emit operationError("mkdir", proc->errorString());
        }
        proc->deleteLater();
    });

    proc->start("ssh", args);
}

QVector<RemoteFileInfo> RemoteConnection::parseLsOutput(const QString &output)
{
    QVector<RemoteFileInfo> files;
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        if (line.startsWith("total")) {
            continue;
        }

        // Parse ls -la output
        // Example: drwxr-xr-x 2 user group 4096 Jan 1 12:00 filename
        QRegularExpression regex("^([d-])([rwx-]{9})\\s+(\\d+)\\s+(\\w+)\\s+(\\w+)\\s+(\\d+)\\s+(.+?)\\s+(\\d{1,2}:\\d{2}|\\w+\\s+\\d+\\s+\\d+)\\s+(.+)$");
        QRegularExpressionMatch match = regex.match(line);

        if (match.hasMatch()) {
            RemoteFileInfo info;
            info.isDirectory = match.captured(1) == "d";
            info.permissions = match.captured(2);
            info.owner = match.captured(4);
            info.group = match.captured(5);
            info.size = match.captured(6).toLongLong();
            info.name = match.captured(9);
            info.path = info.name;

            files.append(info);
        }
    }

    return files;
}

// RemoteFileManager implementation
RemoteFileManager::RemoteFileManager()
{
}

RemoteFileManager::~RemoteFileManager()
{
    qDeleteAll(m_connections);
}

RemoteFileManager& RemoteFileManager::instance()
{
    static RemoteFileManager instance;
    return instance;
}

void RemoteFileManager::addConnection(const QString &name, RemoteConnection *connection)
{
    if (m_connections.contains(name)) {
        delete m_connections[name];
    }
    m_connections[name] = connection;
}

void RemoteFileManager::removeConnection(const QString &name)
{
    if (m_connections.contains(name)) {
        delete m_connections[name];
        m_connections.remove(name);
    }
}

RemoteConnection* RemoteFileManager::connection(const QString &name) const
{
    return m_connections.value(name, nullptr);
}

QStringList RemoteFileManager::connectionNames() const
{
    return m_connections.keys();
}

QString RemoteFileManager::openRemoteFile(const QString &connectionName, const QString &remotePath)
{
    RemoteConnection *conn = connection(connectionName);
    if (!conn) {
        emit error("Connection not found: " + connectionName);
        return QString();
    }

    if (conn->status() != RemoteConnection::Connected) {
        emit error("Not connected to: " + connectionName);
        return QString();
    }

    // Create temporary file
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString fileName = QFileInfo(remotePath).fileName();
    QString localPath = tempDir + "/notepad-supreme-remote-" + fileName;

    // Download file
    conn->downloadFile(remotePath, localPath);

    // Store mapping
    m_remoteFiles[localPath] = remotePath;
    m_fileConnections[localPath] = connectionName;

    connect(conn, &RemoteConnection::downloadComplete,
            this, [this](const QString &remote, const QString &local) {
        emit remoteFileOpened(local, remote);
    });

    return localPath;
}

bool RemoteFileManager::saveRemoteFile(const QString &localPath, const QString &connectionName, const QString &remotePath)
{
    RemoteConnection *conn = connection(connectionName);
    if (!conn) {
        emit error("Connection not found: " + connectionName);
        return false;
    }

    if (conn->status() != RemoteConnection::Connected) {
        emit error("Not connected to: " + connectionName);
        return false;
    }

    // Upload file
    conn->uploadFile(localPath, remotePath);

    connect(conn, &RemoteConnection::uploadComplete,
            this, [this](const QString &remote) {
        emit remoteFileSaved(remote);
    });

    return true;
}

void RemoteFileManager::closeRemoteFile(const QString &localPath)
{
    m_remoteFiles.remove(localPath);
    m_fileConnections.remove(localPath);

    // Delete temporary file
    QFile::remove(localPath);
}

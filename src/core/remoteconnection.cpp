#include "remoteconnection.h"
#include <QRegularExpression>
#include <QFileInfo>
#include <QDir>

// ── RemoteConnection ────────────────────────────────────────────────

RemoteConnection::RemoteConnection(QObject *parent)
    : QObject(parent)
{
}

RemoteConnection::~RemoteConnection() = default;

void RemoteConnection::setConnectionInfo(const ConnectionInfo &info)
{
    m_info = info;
}

RemoteConnection::ConnectionInfo RemoteConnection::connectionInfo() const
{
    return m_info;
}

// --- helper: common SSH options ----------------------------------------

QStringList RemoteConnection::sshArgs() const
{
    QStringList args;
    args << "-o" << "StrictHostKeyChecking=accept-new"
         << "-o" << "BatchMode=yes"
         << "-o" << "ConnectTimeout=10"
         << "-p" << QString::number(m_info.port);

    if (m_info.authMethod == "key" && !m_info.keyPath.isEmpty()) {
        args << "-i" << m_info.keyPath;
    }

    args << QStringLiteral("%1@%2").arg(m_info.username, m_info.host);
    return args;
}

QStringList RemoteConnection::scpArgs() const
{
    QStringList args;
    args << "-o" << "StrictHostKeyChecking=accept-new"
         << "-o" << "BatchMode=yes"
         << "-o" << "ConnectTimeout=10"
         << "-P" << QString::number(m_info.port);

    if (m_info.authMethod == "key" && !m_info.keyPath.isEmpty()) {
        args << "-i" << m_info.keyPath;
    }

    return args;
}

// --- testConnection (synchronous) ----------------------------------------

bool RemoteConnection::testConnection()
{
    QProcess proc;
    QStringList args;
    args << "-o" << "StrictHostKeyChecking=accept-new"
         << "-o" << "BatchMode=yes"
         << "-o" << "ConnectTimeout=5"
         << "-p" << QString::number(m_info.port);

    if (m_info.authMethod == "key" && !m_info.keyPath.isEmpty()) {
        args << "-i" << m_info.keyPath;
    }

    args << QStringLiteral("%1@%2").arg(m_info.username, m_info.host)
         << "echo" << "ok";

    proc.start("ssh", args);
    proc.waitForFinished(10000);

    bool ok = (proc.exitCode() == 0 &&
               proc.exitStatus() == QProcess::NormalExit);

    QString msg = ok
        ? QStringLiteral("Connected successfully")
        : QString::fromUtf8(proc.readAllStandardError()).trimmed();

    if (msg.isEmpty() && !ok) {
        msg = QStringLiteral("Connection failed (exit code %1)")
                  .arg(proc.exitCode());
    }

    emit connectionTestResult(ok, msg);
    return ok;
}

// --- listDirectory (async) -----------------------------------------------

void RemoteConnection::listDirectory(const QString &remotePath)
{
    auto *proc = new QProcess(this);
    QStringList args = sshArgs();
    args << QStringLiteral("ls -laF %1").arg(remotePath);

    connect(proc,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, remotePath](int exitCode, QProcess::ExitStatus) {
        if (exitCode != 0) {
            QString err = QString::fromUtf8(proc->readAllStandardError()).trimmed();
            emit connectionError(
                QStringLiteral("Failed to list %1: %2").arg(remotePath, err));
            proc->deleteLater();
            return;
        }

        QString output = QString::fromUtf8(proc->readAllStandardOutput());
        QStringList entries;

        const QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            if (line.startsWith("total")) {
                continue;
            }

            // ls -laF output example:
            // drwxr-xr-x  2 user group 4096 Mar 27 10:00 subdir/
            // -rw-r--r--  1 user group  123 Mar 27 10:00 file.txt
            QStringList cols = line.split(QRegularExpression("\\s+"),
                                          Qt::SkipEmptyParts);
            if (cols.size() < 9) {
                continue;
            }

            // Name is everything from column 9 onward (handles spaces)
            QString name;
            for (int i = 8; i < cols.size(); ++i) {
                if (!name.isEmpty()) name += ' ';
                name += cols[i];
            }

            // Skip . and ..
            if (name == "." || name == ".." ||
                name == "./" || name == "../") {
                continue;
            }

            bool isDir = cols[0].startsWith('d');
            // Remove trailing / or @ from ls -F
            if (name.endsWith('/') || name.endsWith('@') ||
                name.endsWith('*')) {
                name.chop(1);
            }

            entries << (isDir ? QStringLiteral("d:%1").arg(name)
                              : QStringLiteral("f:%1").arg(name));
        }

        emit directoryListed(entries);
        proc->deleteLater();
    });

    proc->start("ssh", args);
}

// --- downloadFile (async) ------------------------------------------------

void RemoteConnection::downloadFile(const QString &remotePath,
                                    const QString &localPath)
{
    // Ensure the local directory exists
    QDir().mkpath(QFileInfo(localPath).absolutePath());

    auto *proc = new QProcess(this);
    QStringList args = scpArgs();
    args << QStringLiteral("%1@%2:%3")
                .arg(m_info.username, m_info.host, remotePath)
         << localPath;

    connect(proc,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, remotePath, localPath](int exitCode, QProcess::ExitStatus) {
        if (exitCode == 0) {
            emit fileDownloaded(localPath, remotePath);
        } else {
            QString err = QString::fromUtf8(proc->readAllStandardError()).trimmed();
            emit connectionError(
                QStringLiteral("Download failed for %1: %2").arg(remotePath, err));
        }
        proc->deleteLater();
    });

    proc->start("scp", args);
}

// --- uploadFile (async) --------------------------------------------------

void RemoteConnection::uploadFile(const QString &localPath,
                                  const QString &remotePath)
{
    auto *proc = new QProcess(this);
    QStringList args = scpArgs();
    args << localPath
         << QStringLiteral("%1@%2:%3")
                .arg(m_info.username, m_info.host, remotePath);

    connect(proc,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, remotePath](int exitCode, QProcess::ExitStatus) {
        if (exitCode == 0) {
            emit fileUploaded(remotePath);
        } else {
            QString err = QString::fromUtf8(proc->readAllStandardError()).trimmed();
            emit connectionError(
                QStringLiteral("Upload failed for %1: %2").arg(remotePath, err));
        }
        proc->deleteLater();
    });

    proc->start("scp", args);
}

// --- Saved-connection persistence via QSettings --------------------------

QVector<RemoteConnection::ConnectionInfo> RemoteConnection::savedConnections()
{
    QVector<ConnectionInfo> list;
    QSettings settings;
    int count = settings.beginReadArray("RemoteConnections");
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        ConnectionInfo ci;
        ci.name       = settings.value("name").toString();
        ci.host       = settings.value("host").toString();
        ci.port       = settings.value("port", 22).toInt();
        ci.username   = settings.value("username").toString();
        ci.authMethod = settings.value("authMethod", "key").toString();
        ci.keyPath    = settings.value("keyPath").toString();
        ci.lastPath   = settings.value("lastPath", "/").toString();
        list.append(ci);
    }
    settings.endArray();
    return list;
}

void RemoteConnection::saveConnection(const ConnectionInfo &info)
{
    QVector<ConnectionInfo> all = savedConnections();

    // Update existing or append
    bool found = false;
    for (auto &ci : all) {
        if (ci.name == info.name) {
            ci = info;
            found = true;
            break;
        }
    }
    if (!found) {
        all.append(info);
    }

    QSettings settings;
    settings.beginWriteArray("RemoteConnections", all.size());
    for (int i = 0; i < all.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("name",       all[i].name);
        settings.setValue("host",       all[i].host);
        settings.setValue("port",       all[i].port);
        settings.setValue("username",   all[i].username);
        settings.setValue("authMethod", all[i].authMethod);
        settings.setValue("keyPath",    all[i].keyPath);
        settings.setValue("lastPath",   all[i].lastPath);
    }
    settings.endArray();
}

void RemoteConnection::removeConnection(const QString &name)
{
    QVector<ConnectionInfo> all = savedConnections();
    QVector<ConnectionInfo> filtered;
    for (const auto &ci : all) {
        if (ci.name != name) {
            filtered.append(ci);
        }
    }

    QSettings settings;
    settings.beginWriteArray("RemoteConnections", filtered.size());
    for (int i = 0; i < filtered.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("name",       filtered[i].name);
        settings.setValue("host",       filtered[i].host);
        settings.setValue("port",       filtered[i].port);
        settings.setValue("username",   filtered[i].username);
        settings.setValue("authMethod", filtered[i].authMethod);
        settings.setValue("keyPath",    filtered[i].keyPath);
        settings.setValue("lastPath",   filtered[i].lastPath);
    }
    settings.endArray();
}

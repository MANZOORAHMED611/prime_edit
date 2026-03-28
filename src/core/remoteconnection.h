#ifndef REMOTECONNECTION_H
#define REMOTECONNECTION_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QProcess>
#include <QSettings>

/**
 * Remote file operations using SSH/SCP command-line tools.
 *
 * All async file operations run via QProcess and emit signals on completion.
 * Supports key-based and password-less SSH authentication (relies on
 * ssh-agent or key files configured in ~/.ssh/config).
 */
class RemoteConnection : public QObject
{
    Q_OBJECT

public:
    struct ConnectionInfo {
        QString name;
        QString host;
        int port = 22;
        QString username;
        QString authMethod;  // "key" or "password"
        QString keyPath;     // path to SSH private key
        QString lastPath;    // last browsed remote path
    };

    explicit RemoteConnection(QObject *parent = nullptr);
    ~RemoteConnection() override;

    void setConnectionInfo(const ConnectionInfo &info);
    ConnectionInfo connectionInfo() const;

    /**
     * Synchronous connection test.
     * Runs `ssh -o ConnectTimeout=5 user@host echo ok` and returns true
     * if the exit code is 0. Blocks for up to 10 seconds.
     */
    bool testConnection();

    /**
     * List a remote directory asynchronously.
     * Emits directoryListed() with entries prefixed "d:" or "f:".
     */
    void listDirectory(const QString &remotePath);

    /**
     * Download a remote file via scp.
     * Emits fileDownloaded() on success or connectionError() on failure.
     */
    void downloadFile(const QString &remotePath, const QString &localPath);

    /**
     * Upload a local file to a remote path via scp.
     * Emits fileUploaded() on success or connectionError() on failure.
     */
    void uploadFile(const QString &localPath, const QString &remotePath);

    // --- Saved connection persistence via QSettings ---
    static QVector<ConnectionInfo> savedConnections();
    static void saveConnection(const ConnectionInfo &info);
    static void removeConnection(const QString &name);

signals:
    void directoryListed(const QStringList &entries);
    void fileDownloaded(const QString &localPath, const QString &remotePath);
    void fileUploaded(const QString &remotePath);
    void connectionError(const QString &error);
    void connectionTestResult(bool success, const QString &message);

private:
    ConnectionInfo m_info;

    /** Build base ssh argument list: -p PORT -i KEY -o opts USER@HOST */
    QStringList sshArgs() const;

    /** Build base scp argument list: -P PORT -i KEY -o opts */
    QStringList scpArgs() const;
};

#endif // REMOTECONNECTION_H

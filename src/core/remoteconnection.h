#ifndef REMOTECONNECTION_H
#define REMOTECONNECTION_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QProcess>
#include <QTemporaryFile>
#include <QMap>
#include <QDateTime>
#include <QFileInfo>

struct RemoteFileInfo {
    QString path;
    QString name;
    qint64 size;
    bool isDirectory;
    QString permissions;
    QString owner;
    QString group;
    QDateTime modified;
};

class RemoteConnection : public QObject
{
    Q_OBJECT

public:
    enum ConnectionType {
        SFTP,
        SSH,
        SCP
    };

    enum ConnectionStatus {
        Disconnected,
        Connecting,
        Connected,
        Error
    };

    explicit RemoteConnection(QObject *parent = nullptr);
    ~RemoteConnection() override;

    void connectToHost(const QString &host, int port, const QString &username,
                      const QString &password, ConnectionType type = SFTP);
    void disconnect();

    ConnectionStatus status() const { return m_status; }
    QString errorString() const { return m_errorString; }

    // File operations
    void downloadFile(const QString &remotePath, const QString &localPath);
    void uploadFile(const QString &localPath, const QString &remotePath);
    void listDirectory(const QString &path);
    void deleteFile(const QString &remotePath);
    void createDirectory(const QString &remotePath);

    // Connection info
    QString host() const { return m_host; }
    int port() const { return m_port; }
    QString username() const { return m_username; }
    ConnectionType type() const { return m_type; }

signals:
    void connected();
    void disconnected();
    void connectionError(const QString &error);
    void downloadComplete(const QString &remotePath, const QString &localPath);
    void uploadComplete(const QString &remotePath);
    void directoryListed(const QVector<RemoteFileInfo> &files);
    void operationError(const QString &operation, const QString &error);
    void progress(int percentage);

private:
    void setStatus(ConnectionStatus status);
    void executeCommand(const QString &command, const QStringList &args);
    QString buildSftpCommand(const QString &operation, const QStringList &args);
    QString buildScpCommand(const QString &localPath, const QString &remotePath, bool upload);
    QVector<RemoteFileInfo> parseLsOutput(const QString &output);

    ConnectionStatus m_status;
    ConnectionType m_type;
    QString m_host;
    int m_port;
    QString m_username;
    QString m_password;
    QString m_errorString;

    QProcess *m_process;
    QString m_currentOperation;
    QMap<QString, QString> m_pendingDownloads;
    QMap<QString, QString> m_pendingUploads;
};

class RemoteFileManager : public QObject
{
    Q_OBJECT

public:
    static RemoteFileManager& instance();

    // Connection management
    void addConnection(const QString &name, RemoteConnection *connection);
    void removeConnection(const QString &name);
    RemoteConnection* connection(const QString &name) const;
    QStringList connectionNames() const;

    // Remote file editing
    QString openRemoteFile(const QString &connectionName, const QString &remotePath);
    bool saveRemoteFile(const QString &localPath, const QString &connectionName, const QString &remotePath);
    void closeRemoteFile(const QString &localPath);

signals:
    void remoteFileOpened(const QString &localPath, const QString &remotePath);
    void remoteFileSaved(const QString &remotePath);
    void error(const QString &error);

private:
    RemoteFileManager();
    ~RemoteFileManager();
    RemoteFileManager(const RemoteFileManager&) = delete;
    RemoteFileManager& operator=(const RemoteFileManager&) = delete;

    QMap<QString, RemoteConnection*> m_connections;
    QMap<QString, QString> m_remoteFiles;  // local path -> remote path
    QMap<QString, QString> m_fileConnections;  // local path -> connection name
};

#endif // REMOTECONNECTION_H

#ifndef LSPMANAGER_H
#define LSPMANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include "lspclient.h"

class LSPManager : public QObject
{
    Q_OBJECT

public:
    static LSPManager& instance();

    // Get or create LSP client for language
    LSPClient* clientForLanguage(const QString &language);

    // Check if language server is available
    bool hasServerForLanguage(const QString &language) const;

    // Start a language server
    void startServer(const QString &language, const QString &rootPath);

    // Stop a language server
    void stopServer(const QString &language);

    // Stop all servers
    void stopAllServers();

signals:
    void serverStarted(const QString &language);
    void serverStopped(const QString &language);
    void serverError(const QString &language, const QString &error);

private:
    LSPManager();
    ~LSPManager() override;
    LSPManager(const LSPManager&) = delete;
    LSPManager& operator=(const LSPManager&) = delete;

    struct ServerConfig {
        QString command;
        QStringList args;
    };

    ServerConfig getServerConfig(const QString &language) const;

    QMap<QString, LSPClient*> m_clients;
};

#endif // LSPMANAGER_H

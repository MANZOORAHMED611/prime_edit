#include "lspmanager.h"
#include <QDir>

LSPManager::LSPManager()
{
}

LSPManager::~LSPManager()
{
    stopAllServers();
}

LSPManager& LSPManager::instance()
{
    static LSPManager instance;
    return instance;
}

LSPClient* LSPManager::clientForLanguage(const QString &language)
{
    QString lang = language.toLower();

    if (m_clients.contains(lang)) {
        return m_clients[lang];
    }

    return nullptr;
}

bool LSPManager::hasServerForLanguage(const QString &language) const
{
    QString lang = language.toLower();

    // Check if we have a configuration for this language
    if (lang == "python" || lang == "py") return true;
    if (lang == "c++" || lang == "cpp" || lang == "c") return true;
    if (lang == "javascript" || lang == "js" || lang == "typescript" || lang == "ts") return true;
    if (lang == "rust") return true;
    if (lang == "go") return true;

    return false;
}

void LSPManager::startServer(const QString &language, const QString &rootPath)
{
    QString lang = language.toLower();

    // Don't start if already running
    if (m_clients.contains(lang)) {
        LSPClient *existing = m_clients[lang];
        if (existing && existing->isRunning()) {
            return;
        }
    }

    ServerConfig config = getServerConfig(lang);
    if (config.command.isEmpty()) {
        emit serverError(language, "No LSP server configured for " + language);
        return;
    }

    LSPClient *client = new LSPClient(config.command, config.args, this);
    m_clients[lang] = client;

    connect(client, &LSPClient::initialized, this, [this, lang]() {
        emit serverStarted(lang);
    });

    connect(client, &LSPClient::errorOccurred, this, [this, lang](const QString &error) {
        emit serverError(lang, error);
    });

    client->initialize(rootPath);
}

void LSPManager::stopServer(const QString &language)
{
    QString lang = language.toLower();

    if (m_clients.contains(lang)) {
        LSPClient *client = m_clients.take(lang);
        if (client) {
            client->shutdown();
            client->deleteLater();
        }
        emit serverStopped(lang);
    }
}

void LSPManager::stopAllServers()
{
    QStringList languages = m_clients.keys();
    for (const QString &lang : languages) {
        stopServer(lang);
    }
}

LSPManager::ServerConfig LSPManager::getServerConfig(const QString &language) const
{
    ServerConfig config;
    QString lang = language.toLower();

    if (lang == "python" || lang == "py") {
        // Python Language Server (pylsp)
        config.command = "pylsp";
        config.args = QStringList();
    } else if (lang == "c++" || lang == "cpp" || lang == "c") {
        // Clangd for C/C++
        config.command = "clangd";
        config.args = QStringList{"--background-index"};
    } else if (lang == "javascript" || lang == "js" || lang == "typescript" || lang == "ts") {
        // TypeScript Language Server
        config.command = "typescript-language-server";
        config.args = QStringList{"--stdio"};
    } else if (lang == "rust") {
        // Rust Analyzer
        config.command = "rust-analyzer";
        config.args = QStringList();
    } else if (lang == "go") {
        // Go Language Server (gopls)
        config.command = "gopls";
        config.args = QStringList();
    }

    return config;
}

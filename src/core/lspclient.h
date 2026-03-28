#ifndef LSPCLIENT_H
#define LSPCLIENT_H

#include <QObject>
#include <QProcess>
#include <QJsonObject>
#include <QJsonDocument>
#include <QString>
#include <QMap>
#include <QVector>

struct CompletionItem {
    QString label;
    QString detail;
    QString documentation;
    int kind;  // CompletionItemKind from LSP spec
};

struct Diagnostic {
    int line;
    int character;
    int endLine;
    int endCharacter;
    QString message;
    int severity;  // 1=Error, 2=Warning, 3=Info, 4=Hint
};

struct Location {
    QString uri;
    int line;
    int character;
};

class LSPClient : public QObject
{
    Q_OBJECT

public:
    explicit LSPClient(const QString &serverCommand, const QStringList &args, QObject *parent = nullptr);
    ~LSPClient() override;

    // Lifecycle
    void initialize(const QString &rootPath);
    void shutdown();

    // Document synchronization
    void didOpen(const QString &uri, const QString &languageId, const QString &text);
    void didChange(const QString &uri, const QString &text);
    void didClose(const QString &uri);
    void didSave(const QString &uri);

    // Language features
    void completion(const QString &uri, int line, int character);
    void hover(const QString &uri, int line, int character);
    void gotoDefinition(const QString &uri, int line, int character);
    void references(const QString &uri, int line, int character);
    void rename(const QString &uri, int line, int character,
                const QString &newName);

    bool isInitialized() const { return m_initialized; }
    bool isRunning() const;

signals:
    void initialized();
    void shutdownComplete();
    void completionResult(const QVector<CompletionItem> &items);
    void hoverResult(const QString &content);
    void definitionResult(const Location &location);
    void referencesResult(const QVector<Location> &locations);
    void diagnosticsPublished(const QString &uri, const QVector<Diagnostic> &diagnostics);
    void renameResult(const QJsonObject &workspaceEdit);
    void errorOccurred(const QString &error);

private slots:
    void onReadyRead();
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode);

private:
    void sendRequest(const QString &method, const QJsonObject &params);
    void sendNotification(const QString &method, const QJsonObject &params);
    void processMessage(const QJsonObject &message);
    void handleResponse(const QJsonObject &response);
    void handleNotification(const QString &method, const QJsonObject &params);

    QJsonObject parseMessage(const QByteArray &data);

    QProcess *m_process;
    QString m_serverCommand;
    QStringList m_serverArgs;

    int m_nextId;
    bool m_initialized;
    QString m_rootPath;

    QByteArray m_buffer;
    QMap<int, QString> m_pendingRequests;  // id -> method
    QMap<QString, int> m_documentVersions;  // uri -> version counter
};

#endif // LSPCLIENT_H

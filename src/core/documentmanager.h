#ifndef DOCUMENTMANAGER_H
#define DOCUMENTMANAGER_H

#include <QObject>
#include <QList>
#include <QTimer>
#include <QFileSystemWatcher>
#include <QDateTime>
#include <QHash>
#include "document.h"

class DocumentManager : public QObject
{
    Q_OBJECT

public:
    static DocumentManager &instance();

    Document *createDocument();
    Document *openDocument(const QString &filePath);
    bool closeDocument(Document *document, bool force = false);
    void closeAllDocuments();

    Document *findDocument(const QString &filePath) const;
    QList<Document*> documents() const { return m_documents; }
    int documentCount() const { return m_documents.size(); }

    // Recent files
    QStringList recentFiles() const { return m_recentFiles; }
    void addRecentFile(const QString &filePath);
    void clearRecentFiles();

    // Recovery
    void startRecoveryTimer();
    void stopRecoveryTimer();
    QStringList recoverableDocuments() const;
    void recoverDocuments(const QStringList &paths);

signals:
    void documentCreated(Document *document);
    void documentOpened(Document *document);
    void documentClosed(Document *document);
    void documentModifiedChanged(Document *document);
    void recentFilesChanged();
    void fileExternallyModified(const QString &filePath);

private:
    explicit DocumentManager(QObject *parent = nullptr);
    ~DocumentManager() override;

    QList<Document*> m_documents;
    QStringList m_recentFiles;
    QTimer m_recoveryTimer;
    QFileSystemWatcher *m_fileWatcher;
    int m_untitledCount = 0;

    // What our own last save wrote to each path. A watcher event whose current
    // file still matches is the echo of that save, not an external change.
    struct SaveRecord {
        QDateTime mtime;
        qint64 size = 0;
        QByteArray sha1;
    };
    QHash<QString, SaveRecord> m_ownSaves;

    void trackDocument(Document *doc);
    bool matchesOwnSave(const QString &path) const;

    static const int MAX_RECENT_FILES = 20;
    static const int RECOVERY_INTERVAL_MS = 5000;

private slots:
    void onDocumentModifiedChanged(bool modified);
    void saveRecoveryData();
    void onFileChanged(const QString &path);
    void onDocumentSaved(const QString &path, const QByteArray &sha1, qint64 size);
};

#endif // DOCUMENTMANAGER_H

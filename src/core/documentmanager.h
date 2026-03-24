#ifndef DOCUMENTMANAGER_H
#define DOCUMENTMANAGER_H

#include <QObject>
#include <QList>
#include <QTimer>
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

private:
    explicit DocumentManager(QObject *parent = nullptr);
    ~DocumentManager() override;

    QList<Document*> m_documents;
    QStringList m_recentFiles;
    QTimer m_recoveryTimer;
    int m_untitledCount = 0;

    static const int MAX_RECENT_FILES = 20;
    static const int RECOVERY_INTERVAL_MS = 5000;

private slots:
    void onDocumentModifiedChanged(bool modified);
    void saveRecoveryData();
};

#endif // DOCUMENTMANAGER_H

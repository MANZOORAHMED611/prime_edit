#include "documentmanager.h"
#include "utils/settings.h"
#include <QDir>
#include <QStandardPaths>

DocumentManager &DocumentManager::instance()
{
    static DocumentManager instance;
    return instance;
}

DocumentManager::DocumentManager(QObject *parent)
    : QObject(parent)
{
    connect(&m_recoveryTimer, &QTimer::timeout, this, &DocumentManager::saveRecoveryData);

    // Load recent files from settings
    m_recentFiles = Settings::instance().recentFiles();
}

DocumentManager::~DocumentManager()
{
    stopRecoveryTimer();

    // Save recent files
    Settings::instance().setRecentFiles(m_recentFiles);
}

Document *DocumentManager::createDocument()
{
    Document *doc = new Document(this);
    m_documents.append(doc);

    connect(doc, &Document::modifiedChanged, this, &DocumentManager::onDocumentModifiedChanged);

    emit documentCreated(doc);
    return doc;
}

Document *DocumentManager::openDocument(const QString &filePath)
{
    // Check if already open
    Document *existing = findDocument(filePath);
    if (existing) {
        return existing;
    }

    Document *doc = new Document(this);
    if (!doc->load(filePath)) {
        delete doc;
        return nullptr;
    }

    m_documents.append(doc);
    addRecentFile(filePath);

    connect(doc, &Document::modifiedChanged, this, &DocumentManager::onDocumentModifiedChanged);

    emit documentOpened(doc);
    return doc;
}

bool DocumentManager::closeDocument(Document *document, bool force)
{
    if (!m_documents.contains(document)) {
        return false;
    }

    if (!force && document->isModified()) {
        return false;  // Caller should handle prompting
    }

    document->clearRecovery();
    m_documents.removeOne(document);
    emit documentClosed(document);
    document->deleteLater();

    return true;
}

void DocumentManager::closeAllDocuments()
{
    while (!m_documents.isEmpty()) {
        closeDocument(m_documents.first(), true);
    }
}

Document *DocumentManager::findDocument(const QString &filePath) const
{
    for (Document *doc : m_documents) {
        if (doc->filePath() == filePath) {
            return doc;
        }
    }
    return nullptr;
}

void DocumentManager::addRecentFile(const QString &filePath)
{
    m_recentFiles.removeAll(filePath);
    m_recentFiles.prepend(filePath);

    while (m_recentFiles.size() > MAX_RECENT_FILES) {
        m_recentFiles.removeLast();
    }

    Settings::instance().setRecentFiles(m_recentFiles);
    emit recentFilesChanged();
}

void DocumentManager::clearRecentFiles()
{
    m_recentFiles.clear();
    Settings::instance().setRecentFiles(m_recentFiles);
    emit recentFilesChanged();
}

void DocumentManager::startRecoveryTimer()
{
    m_recoveryTimer.start(RECOVERY_INTERVAL_MS);
}

void DocumentManager::stopRecoveryTimer()
{
    m_recoveryTimer.stop();
}

QStringList DocumentManager::recoverableDocuments() const
{
    QStringList result;
    QString recoveryPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/recovery";
    QDir dir(recoveryPath);

    if (dir.exists()) {
        QStringList files = dir.entryList(QStringList() << "*.recovery", QDir::Files);
        for (const QString &file : files) {
            result.append(dir.absoluteFilePath(file));
        }
    }

    return result;
}

void DocumentManager::recoverDocuments(const QStringList &paths)
{
    for (const QString &path : paths) {
        Document *doc = createDocument();
        doc->loadRecovery();
    }
}

void DocumentManager::onDocumentModifiedChanged(bool modified)
{
    Document *doc = qobject_cast<Document*>(sender());
    if (doc) {
        emit documentModifiedChanged(doc);
    }
}

void DocumentManager::saveRecoveryData()
{
    for (Document *doc : m_documents) {
        if (doc->isModified()) {
            doc->saveRecovery();
        }
    }
}

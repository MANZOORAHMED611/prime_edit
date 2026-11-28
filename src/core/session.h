#ifndef SESSION_H
#define SESSION_H

#include <QString>
#include <QJsonObject>
#include <QStringList>
#include <QList>
#include <QSet>

class MainWindow;
class Editor;

class Session
{
public:
    static Session &instance();

    void save(MainWindow *window);
    void restore(MainWindow *window);
    // Re-takes the corrupt-file hold baseline from the window as it is now.
    // No-op when no hold is active (spec P1.10 3.2).
    void rebaseHold(MainWindow *window);

    // Named sessions
    QStringList savedSessions() const;
    void saveSession(const QString &name, MainWindow *window);
    void loadSession(const QString &name, MainWindow *window);
    void deleteSession(const QString &name);

    // Unsaved document persistence (spec 3 D4)
    // Applies the recovery table to sessions/unsaved{,.new,.old,_tmp}. Runs
    // under the single-instance lock before any window exists; idempotent.
    void recoverCacheDirectories();
    // Every launch: places each cached draft in an editor. Never deletes the
    // cache; an unparsable entry is moved to sessions/unsaved.corrupt/.
    void restoreDrafts(MainWindow *window);
    // Only under the "restore previous session" preference: reopens files.
    void restoreFiles(MainWindow *window);
    // Replaces the cached set atomically with every editor's draft (both
    // panes) plus every draft that restore could not place. False only when
    // there was something to store and the replace failed (spec P1.11 3.2.1).
    // While restoreDrafts runs it writes nothing and returns true (3.3.2).
    bool saveUnsavedDocuments(MainWindow *window);
    // 1.0.0 name, kept for the frozen validation suites: restoreDrafts().
    void restoreUnsavedDocuments(MainWindow *window);
    void clearUnsavedCache();

private:
    Session();

    QString sessionFilePath() const;
    QString namedSessionPath(const QString &name) const;
    QString unsavedCacheDir() const;
    QJsonObject windowToJson(MainWindow *window) const;
    void jsonToWindow(const QJsonObject &json, MainWindow *window);
    QJsonObject readSessionFile(const QString &path, bool *corrupt) const;

    // Set when restore() found session.json unparsable (spec D10). The held
    // baseline is the geometry-free projection (P1.10 3.2): window placement
    // alone is not a user change.
    bool m_holdSessionWrites = false;
    QJsonObject m_heldSession;
    // JSON of the last successful session.json write; a tick that would
    // write the same bytes again is skipped (P1.10 3.1).
    QJsonObject m_lastWritten;
    static QJsonObject holdProjection(QJsonObject json);

    // Drafts read from the cache that no editor holds. The writer keeps them.
    QList<QJsonObject> m_unplacedDrafts;
    // Cache entry names this process wrote or restore read. An entry on disk
    // outside this set was never placed, so the writer adopts it.
    QSet<QString> m_seenEntries;
    // Set for the whole body of restoreDrafts. readCacheEntries has marked
    // every entry seen before any is placed, so a write then would drop the
    // drafts not yet placed (spec P1.11 F3).
    bool m_restoringDrafts = false;

    QList<QJsonObject> readCacheEntries(const QSet<QString> &skip);
    void moveToCorruptDir(const QString &path) const;
    bool placeDraft(MainWindow *window, const QJsonObject &draft, QSet<Editor *> *placed);
    Editor *untitledEditorFor(MainWindow *window, const QString &original,
                              const QString &notice);
    QJsonObject draftEntry(Editor *editor) const;
    bool replaceCacheSet(const QList<QJsonObject> &entries);
};

#endif // SESSION_H

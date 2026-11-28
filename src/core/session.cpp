#include "session.h"
#include "ui/mainwindow.h"
#include "ui/editor.h"
#include "ui/tabwidget.h"
#include "core/document.h"
#include "utils/fileutils.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUuid>
#include <QScrollBar>
#include <QFileInfo>
#include <QTabWidget>

Session &Session::instance()
{
    static Session instance;
    return instance;
}

Session::Session()
{
}

void Session::save(MainWindow *window)
{
    const QJsonObject json = windowToJson(window);
    // After a corrupt session.json, write only once the user state (files,
    // bookmarks, folds, active tab) differs from the held baseline.
    if (m_holdSessionWrites && holdProjection(json) == m_heldSession) {
        return;
    }
    m_holdSessionWrites = false;
    if (json == m_lastWritten) {
        return;
    }
    // Only a successful write is remembered, so a failed one is retried.
    if (FileUtils::writeFileAtomic(sessionFilePath(), QJsonDocument(json).toJson())) {
        m_lastWritten = json;
    }
}

void Session::rebaseHold(MainWindow *window)
{
    if (m_holdSessionWrites) {
        m_heldSession = holdProjection(windowToJson(window));
    }
}

// Geometry changes with window-manager placement, not user action.
QJsonObject Session::holdProjection(QJsonObject json)
{
    json.remove("geometry");
    return json;
}

// Returns the parsed object, or an empty one after copying an unparsable file
// to "<file>.corrupt-<timestamp>". The original is never modified.
QJsonObject Session::readSessionFile(const QString &path, bool *corrupt) const
{
    *corrupt = false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isObject()) {
        return doc.object();
    }
    FileUtils::preserveCorruptCopy(path);
    *corrupt = true;
    return QJsonObject();
}

// Drafts first, so a file reopened from the session finds its draft already
// placed; a corrupt session.json no longer stops draft restore.
void Session::restore(MainWindow *window)
{
    restoreDrafts(window);
    restoreFiles(window);
}

void Session::restoreFiles(MainWindow *window)
{
    bool corrupt = false;
    const QJsonObject json = readSessionFile(sessionFilePath(), &corrupt);
    if (corrupt) {
        m_heldSession = holdProjection(windowToJson(window));
        m_holdSessionWrites = true;
    }
    if (corrupt || !QFile::exists(sessionFilePath())) {
        return;
    }
    jsonToWindow(json, window);
}

QStringList Session::savedSessions() const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sessions";
    QDir dir(path);
    if (!dir.exists()) {
        return QStringList();
    }

    QStringList result;
    for (const QString &file : dir.entryList(QStringList() << "*.json", QDir::Files)) {
        result.append(file.chopped(5));  // Remove .json
    }
    return result;
}

void Session::saveSession(const QString &name, MainWindow *window)
{
    // An explicit save by name is the user's change, so it always writes.
    FileUtils::writeFileAtomic(namedSessionPath(name),
                               QJsonDocument(windowToJson(window)).toJson());
}

void Session::loadSession(const QString &name, MainWindow *window)
{
    bool corrupt = false;
    const QJsonObject json = readSessionFile(namedSessionPath(name), &corrupt);
    if (corrupt || !QFile::exists(namedSessionPath(name))) {
        return;
    }
    jsonToWindow(json, window);
}

void Session::deleteSession(const QString &name)
{
    QFile::remove(namedSessionPath(name));
}

QString Session::sessionFilePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/session.json";
}

QString Session::namedSessionPath(const QString &name) const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sessions";
    QDir().mkpath(path);
    return path + "/" + name + ".json";
}

QJsonObject Session::windowToJson(MainWindow *window) const
{
    QJsonObject json;

    // Window geometry
    QJsonObject geometry;
    geometry["x"] = window->x();
    geometry["y"] = window->y();
    geometry["width"] = window->width();
    geometry["height"] = window->height();
    geometry["maximized"] = window->isMaximized();
    json["geometry"] = geometry;

    // Open files
    QJsonArray files;
    // Both panes: a file moved to the second view is still part of the session.
    for (Editor *editor : window->allEditors()) {
        if (!editor->document() || editor->document()->isUntitled()) {
            continue;
        }

        QJsonObject docObj;
        docObj["path"] = editor->document()->filePath();

        QJsonArray bookmarksArray;
        for (int bm : editor->bookmarks()) {
            bookmarksArray.append(bm);
        }
        docObj["bookmarks"] = bookmarksArray;

        QJsonArray foldsArray;
        for (int foldLine : editor->foldedRegions()) {
            foldsArray.append(foldLine);
        }
        docObj["foldedRegions"] = foldsArray;

        files.append(docObj);
    }
    json["files"] = files;

    // Active tab
    json["activeTab"] = window->currentTabIndex();

    return json;
}

void Session::jsonToWindow(const QJsonObject &json, MainWindow *window)
{
    // Restore geometry
    if (json.contains("geometry")) {
        QJsonObject geometry = json["geometry"].toObject();
        window->move(geometry["x"].toInt(), geometry["y"].toInt());
        window->resize(geometry["width"].toInt(), geometry["height"].toInt());
        if (geometry["maximized"].toBool()) {
            window->showMaximized();
        }
    }

    // Restore files
    if (json.contains("files")) {
        QJsonArray files = json["files"].toArray();
        for (const QJsonValue &value : files) {
            QJsonObject docObj = value.toObject();
            QString path = docObj["path"].toString();
            if (path.isEmpty()) {
                continue;
            }
            window->openFile(path);

            // Get the editor that was just opened (it will be the current tab)
            Editor *editor = window->currentEditor();
            if (!editor) {
                continue;
            }

            // Restore bookmarks
            QJsonArray bookmarksArray = docObj["bookmarks"].toArray();
            for (const QJsonValue &v : bookmarksArray) {
                editor->toggleBookmark(v.toInt());
            }

            // Restore fold state
            QJsonArray foldsArray = docObj["foldedRegions"].toArray();
            for (const QJsonValue &v : foldsArray) {
                editor->foldAt(v.toInt());
            }
        }
    }

    // Restore active tab
    if (json.contains("activeTab")) {
        window->setCurrentTabIndex(json["activeTab"].toInt());
    }
}

namespace {

QString lineEndingName(Document::LineEnding ending)
{
    switch (ending) {
    case Document::Windows: return QStringLiteral("CRLF");
    case Document::ClassicMac: return QStringLiteral("CR");
    case Document::Unix: break;
    }
    return QStringLiteral("LF");
}

Document::LineEnding lineEndingFromName(const QString &name)
{
    if (name == QLatin1String("CRLF")) return Document::Windows;
    if (name == QLatin1String("CR")) return Document::ClassicMac;
    return Document::Unix;
}

// The title the tab shows, from whichever pane holds the editor.
QString tabTitle(Editor *editor)
{
    for (QWidget *w = editor->parentWidget(); w; w = w->parentWidget()) {
        if (auto *tabs = qobject_cast<QTabWidget *>(w)) {
            const int index = tabs->indexOf(editor);
            if (index >= 0) {
                return tabs->tabText(index);
            }
        }
    }
    return editor->document()->displayName();
}

// The editor's QTextDocument, not the PieceTable, holds what the user typed:
// the PieceTable is only synced on save. Reading it lost untitled drafts.
bool needsCaching(Editor *editor)
{
    Document *doc = editor->document();
    if (!doc) {
        return false;
    }
    const bool hasContent = !editor->QPlainTextEdit::document()->isEmpty();
    return doc->isModified() || (doc->isUntitled() && hasContent);
}

bool moveTree(const QString &from, const QString &to)
{
    return QDir().rename(from, to);
}

} // namespace

QString Session::unsavedCacheDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + "/sessions/unsaved";
}

// Spec 3 D4 recovery table. Each state is a point where a cache write or a
// v1.0.0 swap was interrupted; every branch keeps the most complete set.
void Session::recoverCacheDirectories()
{
    const QString base = unsavedCacheDir();
    const QString fresh = base + ".new";
    const QString old = base + ".old";
    const QString legacy = base + "_tmp";
    QDir().mkpath(QFileInfo(base).absolutePath());

    QDir(fresh).removeRecursively();  // incomplete write: the old set is whole
    if (QDir(old).exists()) {
        if (QDir(base).exists()) {
            QDir(old).removeRecursively();  // the final delete did not finish
        } else {
            moveTree(old, base);  // crash between the two renames
        }
    }
    if (QDir(legacy).exists()) {
        if (!QDir(base).exists()) {
            moveTree(legacy, base);
        } else {
            bool copied = true;
            for (const QString &name : QDir(legacy).entryList(QDir::Files)) {
                if (!QFile::exists(base + "/" + name)) {
                    copied = QFile::copy(legacy + "/" + name, base + "/" + name) && copied;
                }
            }
            if (copied) {
                QDir(legacy).removeRecursively();
            }
        }
    }
    FileUtils::fsyncDirectory(QFileInfo(base).absolutePath());
}

void Session::moveToCorruptDir(const QString &path) const
{
    const QString dir = unsavedCacheDir() + ".corrupt";
    QDir().mkpath(dir);
    QString target = dir + "/" + QFileInfo(path).fileName();
    if (QFile::exists(target)) {
        target += "." + QUuid::createUuid().toString(QUuid::Id128);
    }
    if (!QFile::rename(path, target) && QFile::copy(path, target)) {
        QFile::remove(path);
    }
}

// Parsed entries of the cached set, skipping names in `skip`. An entry that
// does not parse is moved aside and kept, never dropped.
QList<QJsonObject> Session::readCacheEntries(const QSet<QString> &skip)
{
    QList<QJsonObject> entries;
    QDir dir(unsavedCacheDir());
    for (const QString &name : dir.entryList({"*.json"}, QDir::Files, QDir::Name)) {
        if (skip.contains(name)) {
            continue;
        }
        m_seenEntries.insert(name);
        QFile file(dir.absoluteFilePath(name));
        const QJsonDocument doc = file.open(QIODevice::ReadOnly)
            ? QJsonDocument::fromJson(file.readAll()) : QJsonDocument();
        file.close();
        if (!doc.isObject()) {
            moveToCorruptDir(dir.absoluteFilePath(name));
            continue;
        }
        entries.append(doc.object());
    }
    return entries;
}

void Session::restoreDrafts(MainWindow *window)
{
    recoverCacheDirectories();
    m_unplacedDrafts.clear();
    QSet<Editor *> placed;
    for (const QJsonObject &draft : readCacheEntries({})) {
        if (!placeDraft(window, draft, &placed)) {
            m_unplacedDrafts.append(draft);
        }
    }
}

// A new untitled tab for a draft whose file cannot take it. It keeps the path
// as the "originalFilePath" property (Save As default, and re-cached).
Editor *Session::untitledEditorFor(MainWindow *window, const QString &original,
                                   const QString &notice)
{
    window->newFile();
    Editor *editor = window->currentEditor();
    if (!editor || !editor->document()) {
        return nullptr;
    }
    if (!original.isEmpty()) {
        editor->document()->setProperty("originalFilePath", original);
        window->showNotification(notice.arg(original));
    }
    return editor;
}

void Session::restoreUnsavedDocuments(MainWindow *window)
{
    restoreDrafts(window);
}

// The cached text goes only into the editor whose document is the original
// file. If it cannot be opened, a new untitled tab takes the text and keeps the
// path as the "originalFilePath" property (Save As default, and re-cached).
bool Session::placeDraft(MainWindow *window, const QJsonObject &draft, QSet<Editor *> *placed)
{
    const QString original = draft["originalFilePath"].toString();
    Editor *editor = nullptr;
    QString notice =
        QObject::tr("%1 could not be opened. Its unsaved text was restored in a new tab.");
    if (!original.isEmpty() && QFileInfo(original).isReadable()) {
        Editor *opened = window->openFile(original);
        // openFile returns the existing editor for an open path. If that editor
        // already holds a draft from this restore (two cached drafts of one
        // file), never overwrite it.
        if (opened && placed->contains(opened)) {
            notice = QObject::tr("%1 had more than one unsaved draft. "
                                 "This one was restored in a new tab.");
        } else if (opened && opened->document() && opened->document()->filePath() == original) {
            editor = opened;
        }
    }
    if (!editor) {
        editor = untitledEditorFor(window, original, notice);
        if (!editor) {
            return false;
        }
    }
    placed->insert(editor);
    Document *doc = editor->document();
    if (!draft["encoding"].toString().isEmpty()) {
        doc->setEncoding(draft["encoding"].toString());
    }
    if (draft.contains("lineEnding")) {
        doc->setLineEnding(lineEndingFromName(draft["lineEnding"].toString()));
    }
    const QString text = draft["text"].toString();
    // Modified clears the clean index, so undo cannot make the draft look saved.
    editor->runSyncedEdit(SyncTarget::Modified, [editor, &text]() {
        editor->document()->setText(text);
        editor->syncFromDocument();
    });
    return true;
}

QJsonObject Session::draftEntry(Editor *editor) const
{
    Document *doc = editor->document();
    QJsonObject obj;
    obj["text"] = editor->rawText();  // keeps U+00A0 and U+2028 (spec D11)
    obj["cursorPosition"] = editor->textCursor().position();
    obj["scrollPosition"] = editor->verticalScrollBar()->value();
    obj["title"] = tabTitle(editor);
    obj["originalFilePath"] = doc->isUntitled()
        ? doc->property("originalFilePath").toString() : doc->filePath();
    obj["encoding"] = doc->encoding();
    obj["language"] = doc->language();
    obj["lineEnding"] = lineEndingName(doc->lineEnding());
    return obj;
}

bool Session::saveUnsavedDocuments(MainWindow *window)
{
    // Entries on disk this process never wrote or restored were never placed
    // (restore has not run): keep them rather than replace them.
    m_unplacedDrafts += readCacheEntries(m_seenEntries);

    QList<QJsonObject> entries;
    for (Editor *editor : window->allEditors()) {
        if (needsCaching(editor)) {
            entries.append(draftEntry(editor));
        }
    }
    entries += m_unplacedDrafts;
    replaceCacheSet(entries);
    return true;  // TODO(P1.11-B): placeholder, the result is not reported yet
}

// Write unsaved.new/, then swap it in by two renames. Any failure before the
// second rename leaves the previous set as unsaved/ (see the recovery table).
bool Session::replaceCacheSet(const QList<QJsonObject> &entries)
{
    const QString base = unsavedCacheDir();
    const QString fresh = base + ".new";
    const QString old = base + ".old";
    QDir(fresh).removeRecursively();
    if (!QDir().mkpath(fresh)) {
        return false;
    }
    QSet<QString> written;
    for (const QJsonObject &obj : entries) {
        const QString name = QUuid::createUuid().toString(QUuid::Id128) + ".json";
        if (!FileUtils::writeFileAtomic(fresh + "/" + name, QJsonDocument(obj).toJson())) {
            QDir(fresh).removeRecursively();
            return false;
        }
        written.insert(name);
    }
    FileUtils::fsyncDirectory(fresh);
    QDir(old).removeRecursively();
    if (QDir(base).exists() && !moveTree(base, old)) {
        QDir(fresh).removeRecursively();
        return false;
    }
    if (!moveTree(fresh, base)) {
        moveTree(old, base);
        return false;
    }
    FileUtils::fsyncDirectory(QFileInfo(base).absolutePath());
    QDir(old).removeRecursively();
    m_seenEntries = written;
    return true;
}

void Session::clearUnsavedCache()
{
    QDir dir(unsavedCacheDir());
    dir.removeRecursively();
    dir.mkpath(".");
}

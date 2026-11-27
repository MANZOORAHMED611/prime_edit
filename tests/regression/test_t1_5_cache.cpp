// T1.5 regression tests (spec 2026-11-26-phase1-data-loss-fixes, D4 cache and
// restore, D5 core, D6 restore): the unsaved-document cache is never lost,
// restore places each draft in the right editor, and both panes are covered.
#include "support.h"

#include "core/document.h"
#include "core/session.h"
#include "ui/editor.h"
#include "ui/mainwindow.h"
#include "ui/tabwidget.h"

#include <QCloseEvent>
#include <QDir>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTimer>

#include <sys/stat.h>

namespace {

QString sessionsDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sessions";
}

QString cacheDir()
{
    return sessionsDir() + "/unsaved";
}

void resetCache()
{
    for (const QString &suffix : {"", ".new", ".old", "_tmp", ".corrupt"})
        QDir(cacheDir() + suffix).removeRecursively();
    QDir().mkpath(sessionsDir());
}

QJsonObject entry(const QString &text, const QString &original = QString())
{
    QJsonObject obj;
    obj["text"] = text;
    obj["originalFilePath"] = original;
    obj["encoding"] = "UTF-8";
    obj["title"] = "draft";
    return obj;
}

void writeEntry(const QString &dir, const QString &name, const QJsonObject &obj)
{
    QDir().mkpath(dir);
    QVERIFY(regression::writeBytes(dir + "/" + name, QJsonDocument(obj).toJson()));
}

QStringList cachedTexts()
{
    QStringList texts;
    QDir dir(cacheDir());
    for (const QString &name : dir.entryList({"*.json"}, QDir::Files)) {
        const QJsonDocument doc = QJsonDocument::fromJson(regression::readBytes(dir.filePath(name)));
        texts.append(doc.object()["text"].toString());
    }
    texts.sort();
    return texts;
}

// Texts of every editor that holds something (the initial tab is empty).
QStringList editorTexts(const MainWindow &window)
{
    QStringList texts;
    for (Editor *editor : window.allEditors()) {
        if (!editor->toPlainText().isEmpty())
            texts.append(editor->toPlainText());
    }
    texts.sort();
    return texts;
}

Editor *editorWithText(const MainWindow &window, const QString &text)
{
    for (Editor *editor : window.allEditors()) {
        if (editor->toPlainText() == text)
            return editor;
    }
    return nullptr;
}

// 1.0.0 shows a modal warning when an open fails; close it so the W0-base run
// fails on its assertions instead of blocking.
void dismissModalsWhileAlive(QObject *owner)
{
    auto *timer = new QTimer(owner);
    QObject::connect(timer, &QTimer::timeout, [] {
        if (QWidget *modal = QApplication::activeModalWidget())
            modal->close();
    });
    timer->start(50);
}

void quit(MainWindow &window)
{
    QCloseEvent event;
    QApplication::sendEvent(&window, &event);
}

} // namespace

class TestT15Cache : public QObject
{
    Q_OBJECT

private slots:
    void init() { resetCache(); }
    void cleanupTestCase() { resetCache(); }
    void recoveryTable_data();
    void recoveryTable();
    void unreadableOriginalLandsInNewTab();
    void corruptEntrySurvivesNextWrite();
    void restoredDraftStaysModifiedAfterTypeAndUndo();
    void cachedEncodingAndLineEndingAppliedOnRestore();
    void pane2UntitledDraftCachedOnQuit();
    void sessionSaveIncludesPane2Files();
    void writerKeepsUnrestoredEntriesWithoutTransientDirs();
};

// Each row builds one recovery-table state on disk (spec 3 D4), then restores.
// setup entries are "<dir suffix>/<file>=<text>".
void TestT15Cache::recoveryTable_data()
{
    QTest::addColumn<QStringList>("setup");
    QTest::addColumn<QStringList>("expectedDrafts");
    QTest::addColumn<QStringList>("expectedCache");

    QTest::newRow("unsaved only") << QStringList{"/a.json=A"} << QStringList{"A"} << QStringList{"A"};
    QTest::newRow("incomplete write") << QStringList{"/a.json=A", ".new/b.json=B"}
                                      << QStringList{"A"} << QStringList{"A"};
    QTest::newRow("delete not finished") << QStringList{"/a.json=A", ".old/c.json=C"}
                                         << QStringList{"A"} << QStringList{"A"};
    QTest::newRow("crash between renames") << QStringList{".old/c.json=C"}
                                           << QStringList{"C"} << QStringList{"C"};
    QTest::newRow("v1.0.0 tmp leftover") << QStringList{"_tmp/d.json=D"}
                                         << QStringList{"D"} << QStringList{"D"};
    QTest::newRow("tmp merged into unsaved")
        << QStringList{"/a.json=A", "_tmp/a.json=A-older", "_tmp/d.json=D"}
        << QStringList{"A", "D"} << QStringList{"A", "D"};
}

void TestT15Cache::recoveryTable()
{
    QFETCH(QStringList, setup);
    QFETCH(QStringList, expectedDrafts);
    QFETCH(QStringList, expectedCache);
    for (const QString &item : setup) {
        const QString location = item.section('=', 0, 0);
        writeEntry(cacheDir() + location.section('/', 0, 0), location.section('/', 1),
                   entry(item.section('=', 1)));
    }
    MainWindow window;

    Session::instance().restoreUnsavedDocuments(&window);
    QCOMPARE(editorTexts(window), expectedDrafts);
    QCOMPARE(cachedTexts(), expectedCache);
    for (const QString &suffix : {".new", ".old", "_tmp"})
        QVERIFY2(!QDir(cacheDir() + suffix).exists(), qPrintable(suffix));
}

void TestT15Cache::unreadableOriginalLandsInNewTab()
{
    regression::TempDir dir;
    const QString path = dir.filePath("locked.txt");
    QVERIFY(regression::writeBytes(path, "on disk\n"));
    QVERIFY(::chmod(QFile::encodeName(path).constData(), 0000) == 0);
    writeEntry(cacheDir(), "locked.json", entry("draft for locked file", path));
    MainWindow window;
    dismissModalsWhileAlive(&window);
    Editor *active = window.currentEditor();
    QVERIFY(active);
    active->setPlainText("active tab text");

    Session::instance().restoreUnsavedDocuments(&window);
    ::chmod(QFile::encodeName(path).constData(), 0600);
    QCOMPARE(active->toPlainText(), QString("active tab text"));
    Editor *draft = editorWithText(window, "draft for locked file");
    QVERIFY(draft);
    QVERIFY(draft != active);
    QVERIFY(draft->document()->isUntitled());
    QCOMPARE(draft->document()->property("originalFilePath").toString(), path);
    QVERIFY(draft->document()->isModified());
}

void TestT15Cache::corruptEntrySurvivesNextWrite()
{
    const QByteArray corrupt("{\"text\": \"half writ");
    writeEntry(cacheDir(), "good.json", entry("good draft"));
    QDir().mkpath(cacheDir());
    QVERIFY(regression::writeBytes(cacheDir() + "/bad.json", corrupt));
    MainWindow window;

    Session::instance().restoreUnsavedDocuments(&window);
    Session::instance().saveUnsavedDocuments(&window);
    QCOMPARE(regression::readBytes(cacheDir() + ".corrupt/bad.json"), corrupt);
    QCOMPARE(cachedTexts(), QStringList{"good draft"});
}

void TestT15Cache::restoredDraftStaysModifiedAfterTypeAndUndo()
{
    writeEntry(cacheDir(), "draft.json", entry("restored draft"));
    MainWindow window;
    Session::instance().restoreUnsavedDocuments(&window);
    Editor *editor = editorWithText(window, "restored draft");
    QVERIFY(editor);
    QTextDocument *textDoc = editor->QPlainTextEdit::document();

    QTextCursor cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::End);
    editor->setTextCursor(cursor);
    QTest::keyClick(editor, Qt::Key_X);
    editor->undo();
    QCOMPARE(editor->toPlainText(), QString("restored draft"));
    QVERIFY2(textDoc->isModified(), "undo made the restored draft look clean");
    QVERIFY(editor->document()->isModified());
}

void TestT15Cache::cachedEncodingAndLineEndingAppliedOnRestore()
{
    regression::TempDir dir;
    const QString path = dir.filePath("latin.txt");
    QVERIFY(regression::writeBytes(path, "caf\xE9\n"));
    QJsonObject obj = entry(QString::fromUtf8("café edited"), path);
    obj["encoding"] = "windows-1252";
    obj["lineEnding"] = "CRLF";
    writeEntry(cacheDir(), "latin.json", obj);
    MainWindow window;

    Session::instance().restoreUnsavedDocuments(&window);
    Editor *editor = editorWithText(window, QString::fromUtf8("café edited"));
    QVERIFY(editor);
    QCOMPARE(editor->document()->filePath(), path);
    QCOMPARE(editor->document()->encoding(), QString("windows-1252"));
    QCOMPARE(editor->document()->lineEnding(), Document::Windows);
}

void TestT15Cache::pane2UntitledDraftCachedOnQuit()
{
    MainWindow window;
    window.newFile();
    Editor *editor = window.currentEditor();
    QVERIFY(editor);
    editor->setPlainText("lives in pane two");
    QVERIFY(QMetaObject::invokeMethod(&window, "moveToOtherView"));
    QVERIFY(window.isSplit());
    QVERIFY(window.tabWidget()->indexOf(editor) < 0);

    quit(window);
    QVERIFY(cachedTexts().contains("lives in pane two"));
}

void TestT15Cache::sessionSaveIncludesPane2Files()
{
    regression::TempDir dir;
    const QString first = dir.filePath("one.txt");
    const QString second = dir.filePath("two.txt");
    QVERIFY(regression::writeBytes(first, "1\n"));
    QVERIFY(regression::writeBytes(second, "2\n"));
    MainWindow window;
    QVERIFY(window.openFile(first));
    QVERIFY(window.openFile(second));
    QVERIFY(QMetaObject::invokeMethod(&window, "moveToOtherView"));
    QVERIFY(window.isSplit());

    Session::instance().save(&window);
    const QString sessionFile =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/session.json";
    const QJsonArray files =
        QJsonDocument::fromJson(regression::readBytes(sessionFile)).object()["files"].toArray();
    QStringList paths;
    for (const QJsonValue &value : files)
        paths.append(value.toObject()["path"].toString());
    QVERIFY2(paths.contains(second), qPrintable(paths.join(", ")));
    QVERIFY(paths.contains(first));
}

void TestT15Cache::writerKeepsUnrestoredEntriesWithoutTransientDirs()
{
    // Restore never ran in this process, so this entry was never placed: the
    // writer must carry it, not replace it (spec 3 D4, "Writing the set").
    writeEntry(cacheDir(), "earlier.json", entry("draft restore never placed"));
    MainWindow window;
    QStringList expected;
    for (int i = 0; i < 10; ++i) {
        window.newFile();
        const QString text = QString("draft %1 ").arg(i) + QString(2000, QChar('a' + i));
        window.currentEditor()->setPlainText(text);
        expected.append(text);
    }
    expected.append("draft restore never placed");
    expected.sort();

    QElapsedTimer timer;
    timer.start();
    Session::instance().saveUnsavedDocuments(&window);
    qInfo("cache write with 10 drafts: %lld ms", static_cast<long long>(timer.elapsed()));
    QCOMPARE(cachedTexts(), expected);
    for (const QString &suffix : {".new", ".old", "_tmp"})
        QVERIFY2(!QDir(cacheDir() + suffix).exists(), qPrintable(suffix));
}

PRIMEEDIT_REGRESSION_MAIN(TestT15Cache)

#include "test_t1_5_cache.moc"

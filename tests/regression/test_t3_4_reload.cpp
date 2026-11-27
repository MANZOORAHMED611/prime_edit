// T3.4 regression tests (spec docs/superpowers/specs/2026-11-26-phase1-data-loss-fixes.md,
// D8 UI and D1 reload, section 9.2 row T3.4), plus three items the orchestrator
// folded into T3.4: D11 text commands, conversions refused on read-only
// documents, and the Save As default for a restored draft.
#include "support.h"

#include "core/document.h"
#include "core/documentmanager.h"
#include "ui/editor.h"
#include "ui/mainwindow.h"
#include "ui/notificationbar.h"
#include "ui/tabwidget.h"

#include <QAction>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>

namespace {

bool showActive(QWidget &w)
{
    w.show();
    w.activateWindow();
    return QTest::qWaitForWindowActive(&w);
}

void typeAtEnd(Editor *editor, const QString &text)
{
    editor->setFocus();
    editor->moveCursor(QTextCursor::End);
    QTest::keyClicks(editor, text);
}

NotificationBar *bar(MainWindow &mw) { return mw.findChild<NotificationBar*>(); }

QString notificationText(MainWindow &mw)
{
    NotificationBar *b = bar(mw);
    if (!b || !b->isVisible()) return QString();
    QStringList parts;
    for (QLabel *label : b->findChildren<QLabel*>()) parts << label->text();
    return parts.join(QLatin1Char(' '));
}

QPushButton *visibleButton(MainWindow &mw, const QString &label)
{
    NotificationBar *b = bar(mw);
    if (!b || !b->isVisible()) return nullptr;
    for (QPushButton *button : b->findChildren<QPushButton*>()) {
        if (button->isVisible() && button->text().remove(QLatin1Char('&')) == label) return button;
    }
    return nullptr;
}

// An external program rewriting the file. A one-second pause first, so the
// mtime differs even on filesystems with coarse timestamps.
void writeExternally(const QString &path, const QByteArray &bytes)
{
    QTest::qWait(1100);
    QVERIFY(regression::writeBytes(path, bytes));
}

QMenu *encodingMenu(MainWindow &mw)
{
    for (QAction *a : mw.menuBar()->actions()) {
        if (a->text().remove(QLatin1Char('&')) == QStringLiteral("Encoding")) return a->menu();
    }
    return nullptr;
}

QAction *menuItem(QMenu *menu, const QString &label)
{
    for (QAction *a : menu->actions()) {
        if (a->text().remove(QLatin1Char('&')) == label) return a;
    }
    return nullptr;
}

constexpr QFile::Permissions kReadOnlyFile = QFile::ReadOwner | QFile::ReadGroup | QFile::ReadOther;

} // namespace

class TestT34Reload : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();
    void ownSaveShowsNoBar();
    void externalChangeWithUnsavedEditsShowsBarAndNeverReloadsSilently();
    void backgroundTabGetsTheBarOnActivationAndReloadsItsEditor();
    void textCommandsKeepNoBreakSpace_data();
    void textCommandsKeepNoBreakSpace();
    void conversionsRefusedOnReadOnlyDocuments_data();
    void conversionsRefusedOnReadOnlyDocuments();
    void saveAsDefaultsToARestoredDraftsOriginalPath();
};

void TestT34Reload::cleanup()
{
    DocumentManager::instance().closeAllDocuments();
}

// D8: a save made by PrimeEdit itself never raises the reload bar.
void TestT34Reload::ownSaveShowsNoBar()
{
    regression::TempDir tmp;
    const QString path = tmp.filePath(QStringLiteral("own.txt"));
    QVERIFY(regression::writeBytes(path, "one\n"));
    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.openFile(path);
    QVERIFY(editor);
    typeAtEnd(editor, QStringLiteral("two"));
    mw.saveFile();
    QCOMPARE(regression::readBytes(path), QByteArray("one\ntwo"));
    QTest::qWait(1500);
    QVERIFY2(!notificationText(mw).contains(QStringLiteral("another program")), qPrintable(notificationText(mw)));
}

// D8/D1: an external change while the tab has unsaved edits shows the bar with
// "Reload (discard my changes)" and "Keep mine". Nothing reloads until the user
// chooses; Reload refills the editor clean (runSyncedEdit(Clean)).
void TestT34Reload::externalChangeWithUnsavedEditsShowsBarAndNeverReloadsSilently()
{
    regression::TempDir tmp;
    const QString path = tmp.filePath(QStringLiteral("shared.txt"));
    QVERIFY(regression::writeBytes(path, "base\n"));
    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.openFile(path);
    QVERIFY(editor);
    typeAtEnd(editor, QStringLiteral("mine"));

    writeExternally(path, "theirs 1\n");
    QTRY_VERIFY_WITH_TIMEOUT(visibleButton(mw, QStringLiteral("Keep mine")), 5000);
    QVERIFY2(visibleButton(mw, QStringLiteral("Reload (discard my changes)")), qPrintable(notificationText(mw)));
    QCOMPARE(editor->rawText(), QStringLiteral("base\nmine"));
    visibleButton(mw, QStringLiteral("Keep mine"))->click();
    QCOMPARE(editor->rawText(), QStringLiteral("base\nmine"));
    QVERIFY(editor->document()->isModified());

    writeExternally(path, "theirs 2\n");
    QTRY_VERIFY_WITH_TIMEOUT(visibleButton(mw, QStringLiteral("Reload (discard my changes)")), 5000);
    visibleButton(mw, QStringLiteral("Reload (discard my changes)"))->click();
    QCOMPARE(editor->rawText(), QStringLiteral("theirs 2\n"));
    QVERIFY(!editor->document()->isModified());
    editor->moveCursor(QTextCursor::Start);
    QTest::keyClicks(editor, QStringLiteral("z"));
    editor->undo();
    QVERIFY2(!editor->document()->isModified(), "type plus undo after a reload looks modified");
}

// D8: a change to a background tab does not interrupt the current tab. The bar
// appears when that tab is activated, and Reload refills that tab's editor.
void TestT34Reload::backgroundTabGetsTheBarOnActivationAndReloadsItsEditor()
{
    regression::TempDir tmp;
    const QString pathA = tmp.filePath(QStringLiteral("a.txt"));
    const QString pathB = tmp.filePath(QStringLiteral("b.txt"));
    QVERIFY(regression::writeBytes(pathA, "A old\n"));
    QVERIFY(regression::writeBytes(pathB, "B\n"));
    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editorA = mw.openFile(pathA);
    Editor *editorB = mw.openFile(pathB);
    QVERIFY(editorA && editorB);
    QCOMPARE(mw.currentEditor(), editorB);

    writeExternally(pathA, "A new\n");
    QTest::qWait(1500);
    QVERIFY2(!notificationText(mw).contains(QStringLiteral("a.txt")),
             "the bar for a background tab interrupted the current tab");

    mw.tabWidget()->setCurrentIndex(mw.tabWidget()->indexOf(editorA));
    QTRY_VERIFY_WITH_TIMEOUT(notificationText(mw).contains(QStringLiteral("a.txt")), 3000);
    QPushButton *reload = visibleButton(mw, QStringLiteral("Reload"));
    QVERIFY2(reload, qPrintable(notificationText(mw)));
    reload->click();
    QCOMPARE(editorA->rawText(), QStringLiteral("A new\n"));
    QCOMPARE(editorA->document()->text(), QStringLiteral("A new\n"));
    QVERIFY(!editorA->document()->isModified());
    QCOMPARE(editorB->rawText(), QStringLiteral("B\n"));
}

void TestT34Reload::textCommandsKeepNoBreakSpace_data()
{
    QTest::addColumn<QString>("slot");
    for (const char *slot : {"removeEmptyLines", "removeEmptyLinesWithBlanks",
                             "removeConsecutiveDuplicateLines", "trimLeadingWhitespace",
                             "trimLeadingAndTrailing", "eolToSpace", "removeUnnecessaryBlanks",
                             "tabToSpace", "spaceToTabAll", "spaceToTabLeading"}) {
        QTest::newRow(slot) << QString::fromLatin1(slot);
    }
}

// D11: each text command rebuilt the document from toPlainText(), which turns
// U+00A0 into a space and U+2028 into a line break. They must use rawText().
void TestT34Reload::textCommandsKeepNoBreakSpace()
{
    QFETCH(QString, slot);
    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.currentEditor();
    QVERIFY(editor);
    editor->insertPlainText(QStringLiteral("  a b\n\n\n\tc d e  \n        f g\n"));

    QVERIFY(QMetaObject::invokeMethod(&mw, slot.toLatin1().constData(), Qt::DirectConnection));
    const QString after = editor->rawText();
    QCOMPARE(after.count(QChar(0x00A0)), 3);
    QCOMPARE(after.count(QChar(0x2028)), 1);
}

void TestT34Reload::conversionsRefusedOnReadOnlyDocuments_data()
{
    QTest::addColumn<QByteArray>("bytes");
    QTest::addColumn<bool>("lockFile");
    // A CP932 duplicate decodes but does not re-encode: a lossy, read-only open.
    QTest::newRow("lossy decode") << QByteArray("abc \xED\x40 def\n") << false;
    QTest::newRow("no write permission") << QByteArray("plain\n") << true;
}

// Converting a read-only document only marks it dirty and the save is refused
// anyway, so the Encode items and the conversion slots refuse it up front.
void TestT34Reload::conversionsRefusedOnReadOnlyDocuments()
{
    QFETCH(QByteArray, bytes);
    QFETCH(bool, lockFile);
    regression::TempDir tmp;
    const QString path = tmp.filePath(QStringLiteral("ro.txt"));
    QVERIFY(regression::writeBytes(path, bytes));
    if (lockFile) QVERIFY(QFile::setPermissions(path, kReadOnlyFile));
    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.openFile(path);
    QVERIFY(editor);
    Document *doc = editor->document();

    QMenu *menu = encodingMenu(mw);
    QVERIFY(menu);
    QAction *encode = menuItem(menu, QStringLiteral("Encode in UTF-8-BOM"));
    QVERIFY(encode);
    QVERIFY2(!encode->isEnabled(), "Encode in UTF-8-BOM is enabled for a read-only document");

    const QString encodingBefore = doc->encoding();
    const auto endingBefore = doc->lineEnding();
    mw.convertToUTF8BOM();
    mw.convertToWindows();
    QCOMPARE(doc->encoding(), encodingBefore);
    QCOMPARE(doc->lineEnding(), endingBefore);
    QVERIFY(!doc->isModified());
    QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
}

// T1.5 restores a draft whose original cannot be reopened as an untitled tab
// that keeps the path in "originalFilePath". Save As offers that path.
void TestT34Reload::saveAsDefaultsToARestoredDraftsOriginalPath()
{
    regression::TempDir tmp;
    const QString original = tmp.filePath(QStringLiteral("draft-notes.txt"));
    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.currentEditor();
    QVERIFY(editor);
    editor->insertPlainText(QStringLiteral("draft"));
    editor->document()->setProperty("originalFilePath", original);

    QString offered;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        auto *dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
        if (!dialog) return;
        offered = dialog->selectedFiles().value(0);
        dialog->reject();
    });
    timer.start(20);
    mw.saveFileAs();
    QVERIFY2(offered.endsWith(QStringLiteral("draft-notes.txt")), qPrintable(offered));
}

PRIMEEDIT_REGRESSION_MAIN(TestT34Reload)

#include "test_t3_4_reload.moc"

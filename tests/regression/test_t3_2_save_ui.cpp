// T3.2 regression tests (spec docs/superpowers/specs/2026-11-26-phase1-data-loss-fixes.md,
// D3/D7/D12 UI, section 9.2 row T3.2). Every test drives a real MainWindow.
#include "support.h"

#include "core/document.h"
#include "core/documentmanager.h"
#include "ui/editor.h"
#include "ui/mainwindow.h"
#include "ui/notificationbar.h"
#include "ui/tabwidget.h"

#include <QAbstractButton>
#include <QAction>
#include <QFile>
#include <QMessageBox>
#include <QPushButton>
#include <QTabBar>
#include <QTimer>

namespace {

// Answers every QMessageBox that opens while it is alive: it clicks the button
// whose text contains wantedText (or the wanted standard button). When that
// button is absent it records a miss and closes the box with its escape or
// first button, so a missing button fails the test instead of hanging it.
class PromptAnswerer
{
public:
    explicit PromptAnswerer(QMessageBox::StandardButton wanted) : m_wanted(wanted) { start(); }
    explicit PromptAnswerer(const QString &wantedText) : m_wantedText(wantedText) { start(); }

    int answered() const { return m_answered; }
    int missed() const { return m_missed; }
    QStringList texts() const { return m_texts; }

private:
    void start()
    {
        QObject::connect(&m_timer, &QTimer::timeout, [this]() { answer(); });
        m_timer.start(20);
    }

    QAbstractButton *wantedButton(QMessageBox *box) const
    {
        if (m_wantedText.isEmpty()) return box->button(m_wanted);
        for (QAbstractButton *b : box->buttons()) {
            if (b->text().remove(QLatin1Char('&')).contains(m_wantedText, Qt::CaseInsensitive))
                return b;
        }
        return nullptr;
    }

    void answer()
    {
        auto *box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        if (!box || box->property("answered").toBool()) return;
        box->setProperty("answered", true);
        m_texts << box->text();
        QAbstractButton *button = wantedButton(box);
        if (button) {
            ++m_answered;
        } else {
            ++m_missed;
            button = box->escapeButton() ? box->escapeButton() : box->buttons().value(0);
        }
        if (button) button->click();
    }

    QTimer m_timer;
    QMessageBox::StandardButton m_wanted = QMessageBox::NoButton;
    QString m_wantedText;
    int m_answered = 0;
    int m_missed = 0;
    QStringList m_texts;
};

bool showActive(QWidget &w)
{
    w.show();
    w.activateWindow();
    return QTest::qWaitForWindowActive(&w);
}

TabWidget *secondPane(MainWindow &mw)
{
    for (TabWidget *tw : mw.findChildren<TabWidget*>()) {
        if (tw != mw.tabWidget()) return tw;
    }
    return nullptr;
}

QAction *actionNamed(MainWindow &mw, const QString &label)
{
    for (QAction *a : mw.findChildren<QAction*>()) {
        if (a->text().remove(QLatin1Char('&')) == label) return a;
    }
    return nullptr;
}

// Types text at the end of the editor as keystrokes, so undo sees user edits.
void typeAtEnd(Editor *editor, const QString &text)
{
    editor->setFocus();
    editor->moveCursor(QTextCursor::End);
    QTest::keyClicks(editor, text);
}

QString notificationText(MainWindow &mw)
{
    auto *bar = mw.findChild<NotificationBar*>();
    if (!bar || !bar->isVisible()) return QString();
    QStringList parts;
    for (QLabel *label : bar->findChildren<QLabel*>()) parts << label->text();
    return parts.join(QLatin1Char(' '));
}

// Restores a scratch directory's mode on scope exit so TempDir can remove it.
struct ModeRestorer {
    QString path;
    ~ModeRestorer() { QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner); }
};

constexpr QFile::Permissions kReadOnlyFile = QFile::ReadOwner | QFile::ReadGroup | QFile::ReadOther;
constexpr QFile::Permissions kReadOnlyDir = kReadOnlyFile | QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther;

} // namespace

class TestT32SaveUi : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();
    void failedSaveStaysModifiedAfterTypeAndUndo();
    void readOnlyDirectoryOffersDirectWriteAndSavesOnConfirm();
    void partialViewCtrlSAndConversionsLeaveBytesUnchanged();
    void modifiedDocumentThatBecameReadOnlyPromptsOnClose();
    void closePromptSaveWritesThatTabsFileNotTheFocusedOne();
};

void TestT32SaveUi::cleanup()
{
    // Each test's MainWindow is gone; close its Documents so their file
    // watchers cannot post a message into the next test's notification bar.
    DocumentManager::instance().closeAllDocuments();
    Document::setSizeThresholdsForTest(Document::MEDIUM_FILE_THRESHOLD, Document::LARGE_FILE_THRESHOLD);
}

// D7: a failed save must leave the document modified, also after the user types
// and undoes, and the error dialog must not offer a direct write (0444 target).
void TestT32SaveUi::failedSaveStaysModifiedAfterTypeAndUndo()
{
    regression::TempDir tmp;
    const QString path = tmp.filePath(QStringLiteral("locked.txt"));
    QVERIFY(regression::writeBytes(path, "original\n"));
    QVERIFY(QFile::setPermissions(path, kReadOnlyFile));
    ModeRestorer restore{path};

    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.openFile(path);
    QVERIFY(editor);
    typeAtEnd(editor, QStringLiteral("x"));
    {
        PromptAnswerer ok(QMessageBox::Ok);
        mw.saveFile();
        QCOMPARE(ok.answered() + ok.missed(), 1);
        for (const QString &t : ok.texts()) QVERIFY2(!t.isEmpty(), "the save error dialog has no text");
    }
    QCOMPARE(regression::readBytes(path), QByteArray("original\n"));
    QVERIFY(editor->document()->isModified());

    // Type away from the first edit so undo removes only this one (adjacent
    // typing merges into a single undo step).
    editor->moveCursor(QTextCursor::Start);
    QTest::keyClicks(editor, QStringLiteral("y"));
    editor->undo();
    QVERIFY(editor->rawText().endsWith(QLatin1Char('x')));
    QVERIFY2(editor->document()->isModified(), "type plus undo made a failed save look clean");
}

// D7: a writable file in a 0555 directory cannot be saved atomically. The error
// offers "Save directly", and confirming writes the file and clears modified.
void TestT32SaveUi::readOnlyDirectoryOffersDirectWriteAndSavesOnConfirm()
{
    regression::TempDir tmp;
    const QString dir = tmp.filePath(QStringLiteral("locked-dir"));
    QVERIFY(QDir().mkpath(dir));
    const QString path = dir + QStringLiteral("/note.txt");
    QVERIFY(regression::writeBytes(path, "before\n"));
    QVERIFY(QFile::setPermissions(dir, kReadOnlyDir));
    ModeRestorer restore{dir};

    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.openFile(path);
    QVERIFY(editor);
    typeAtEnd(editor, QStringLiteral("after"));

    PromptAnswerer direct(QStringLiteral("Save directly"));
    mw.saveFile();
    QCOMPARE(direct.missed(), 0);
    QCOMPARE(direct.answered(), 1);
    QCOMPARE(regression::readBytes(path), QByteArray("before\nafter"));
    QVERIFY(!editor->document()->isModified());
}

// D3: a partial view gets the banner in the notification bar, Save, Save As,
// Save All and the conversions are disabled, and Ctrl+S, the save slot and the
// conversion slots all leave the file's bytes and format unchanged.
void TestT32SaveUi::partialViewCtrlSAndConversionsLeaveBytesUnchanged()
{
    regression::TempDir tmp;
    const QString path = tmp.filePath(QStringLiteral("big.log"));
    QByteArray bytes;
    for (int i = 0; i < 400; ++i) bytes += "line of a large log file\n";
    QVERIFY(regression::writeBytes(path, bytes));
    Document::setSizeThresholdsForTest(1024, 4096);

    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.openFile(path);
    QVERIFY(editor);
    Document *doc = editor->document();
    QVERIFY(doc->isPartialView());
    QVERIFY2(notificationText(mw).contains(QStringLiteral("part of"), Qt::CaseInsensitive),
             qPrintable(QStringLiteral("no partial-view banner; bar says: ") + notificationText(mw)));

    for (const QString &label : {QStringLiteral("Save"), QStringLiteral("Save As..."), QStringLiteral("Save All")}) {
        QAction *a = actionNamed(mw, label);
        QVERIFY2(a, qPrintable(label));
        QVERIFY2(!a->isEnabled(), qPrintable(label + QStringLiteral(" is enabled for a partial view")));
    }
    QAction *encode = actionNamed(mw, QStringLiteral("Encode in UTF-8-BOM"));
    QVERIFY(encode);
    QVERIFY2(!encode->isEnabled(), "Encode in UTF-8-BOM is enabled for a partial view");

    const QString encodingBefore = doc->encoding();
    const auto endingBefore = doc->lineEnding();
    PromptAnswerer dismiss(QMessageBox::Ok);
    QTest::keyClick(editor, Qt::Key_S, Qt::ControlModifier);
    mw.saveFile();
    mw.convertToUTF8BOM();
    mw.convertToWindows();
    QCOMPARE(regression::readBytes(path), bytes);
    QCOMPARE(doc->encoding(), encodingBefore);
    QCOMPARE(doc->lineEnding(), endingBefore);
    QVERIFY(!doc->isModified());
}

// D12: both close prompts key on isModified() alone. A modified document that
// later became read-only must still prompt instead of silently discarding.
void TestT32SaveUi::modifiedDocumentThatBecameReadOnlyPromptsOnClose()
{
    regression::TempDir tmp;
    const QString path = tmp.filePath(QStringLiteral("doc.txt"));
    QVERIFY(regression::writeBytes(path, "text\n"));

    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.openFile(path);
    QVERIFY(editor);
    typeAtEnd(editor, QStringLiteral("edit"));
    QVERIFY(editor->document()->isModified());
    editor->document()->setReadOnly(true);

    {
        PromptAnswerer cancel(QMessageBox::Cancel);
        QVERIFY2(!mw.closeFile(), "the modified read-only tab closed without a prompt");
        QCOMPARE(cancel.answered(), 1);
    }
    PromptAnswerer discard(QMessageBox::Discard);
    QVERIFY(mw.closeFile());
    QCOMPARE(discard.answered(), 1);
    QCOMPARE(regression::readBytes(path), QByteArray("text\n"));
}

// Save from a close prompt must save the tab being closed. The old path saved
// whatever editor had focus, so closing a pane-1 tab could write pane 2's file.
void TestT32SaveUi::closePromptSaveWritesThatTabsFileNotTheFocusedOne()
{
    regression::TempDir tmp;
    const QString pathA = tmp.filePath(QStringLiteral("a.txt"));
    const QString pathB = tmp.filePath(QStringLiteral("b.txt"));
    QVERIFY(regression::writeBytes(pathA, "A\n"));
    QVERIFY(regression::writeBytes(pathB, "B\n"));

    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editorA = mw.openFile(pathA);
    Editor *editorB = mw.openFile(pathB);
    QVERIFY(editorA && editorB);
    QVERIFY(QMetaObject::invokeMethod(&mw, "moveToOtherView", Qt::DirectConnection));
    TabWidget *pane1 = mw.tabWidget();
    TabWidget *pane2 = secondPane(mw);
    QVERIFY(pane2);
    QVERIFY(pane2->indexOf(editorB) >= 0);
    typeAtEnd(editorA, QStringLiteral("a-edit"));
    typeAtEnd(editorB, QStringLiteral("b-edit"));
    QTRY_VERIFY(editorB->hasFocus());

    // Another window takes activation, so focus cannot follow the close prompt.
    QWidget other;
    QVERIFY(showActive(other));

    PromptAnswerer save(QMessageBox::Save);
    emit pane1->tabBar()->tabCloseRequested(pane1->indexOf(editorA));
    QCOMPARE(save.answered(), 1);
    QCOMPARE(regression::readBytes(pathA), QByteArray("A\na-edit"));
    QCOMPARE(regression::readBytes(pathB), QByteArray("B\n"));
    QCOMPARE(pane1->indexOf(editorA), -1);
    QVERIFY(editorB->document()->isModified());
}

PRIMEEDIT_REGRESSION_MAIN(TestT32SaveUi)

#include "test_t3_2_save_ui.moc"

// P1.11-B regression tests (spec docs/superpowers/specs/2026-11-27-pr5-review-data-safety.md,
// section 5 cases 5-9): a failed draft-cache write at quit asks before discarding
// (F2), no draft write runs while drafts are being restored (F3), and a draft
// never lands in a read-only document (F4).
#include "support.h"

#include "core/document.h"
#include "core/session.h"
#include "ui/editor.h"
#include "ui/mainwindow.h"
#include "ui/notificationbar.h"

#include <QAbstractButton>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTimer>

#include <sys/stat.h>

namespace {

const QString kScratchMarker = QStringLiteral("/home-test_t1_11b_drafts");

QString sessionsDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sessions";
}

QString cacheDir()
{
    return sessionsDir() + "/unsaved";
}

bool setMode(const QString &path, mode_t mode)
{
    return ::chmod(QFile::encodeName(path).constData(), mode) == 0;
}

void resetCache()
{
    setMode(sessionsDir(), 0700);
    for (const QString &suffix : {"", ".new", ".old", "_tmp", ".corrupt"})
        QDir(cacheDir() + suffix).removeRecursively();
    QDir().mkpath(cacheDir());
}

// Makes the cache parent unwritable for one case and restores it however the
// case ends, so the next case and the scratch HOME cleanup still work.
struct UnwritableCache {
    UnwritableCache() { ok = setMode(sessionsDir(), 0500); }
    ~UnwritableCache() { setMode(sessionsDir(), 0700); }
    bool ok = false;
};

int cachedEntryCount()
{
    return QDir(cacheDir()).entryList({"*.json"}, QDir::Files).size();
}

QJsonObject draftEntry(const QString &text, const QString &original)
{
    return QJsonObject{{"text", text}, {"originalFilePath", original},
                       {"encoding", "UTF-8"}, {"title", "draft"}};
}

void writeEntry(const QString &name, const QJsonObject &obj)
{
    QVERIFY(regression::writeBytes(cacheDir() + "/" + name, QJsonDocument(obj).toJson()));
}

// As in test_t3_2_save_ui.cpp (texts here also keep the informative text): answers every QMessageBox that opens while it
// is alive by clicking the button whose text contains wantedText (or the wanted
// standard button). A missing button is recorded as a miss and the box is closed
// with its escape or first button, so the test fails instead of hanging.
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
        m_texts << box->text() + QLatin1Char('\n') + box->informativeText();
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

QTimer *draftTimer(MainWindow &window)
{
    return window.findChild<QTimer*>(QStringLiteral("draftBackupTimer"));
}

// A shown window whose only tab holds unsaved text.
void typeUnsaved(MainWindow &window, const QString &text)
{
    Editor *editor = window.currentEditor();
    QVERIFY(editor);
    editor->setFocus();
    QTest::keyClicks(editor, text);
    QVERIFY(editor->document()->isModified());
}

bool notificationVisible(MainWindow &window)
{
    auto *bar = window.findChild<NotificationBar*>();
    return bar && bar->isVisible();
}

void hideNotification(MainWindow &window)
{
    if (auto *bar = window.findChild<NotificationBar*>())
        bar->hide();
}

Editor *editorForFile(MainWindow &window, const QString &path)
{
    for (Editor *editor : window.allEditors()) {
        if (editor->document() && editor->document()->filePath() == path)
            return editor;
    }
    return nullptr;
}

Editor *editorWithText(MainWindow &window, const QString &text)
{
    for (Editor *editor : window.allEditors()) {
        if (editor->toPlainText() == text)
            return editor;
    }
    return nullptr;
}

} // namespace

class TestT111bDrafts : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init() { resetCache(); }
    void cleanupTestCase() { resetCache(); }
    void quitCancelledWhenDraftsCannotBeStored();
    void quitCancelledBeforeAutosaveLeavesTimersStopped();
    void quitConfirmedDiscardsWhenDraftsCannotBeStored();
    void nothingAtRiskQuitsWithoutPrompt();
    void periodicFailureNotifiesOncePerFailureRun();
    void saveDuringRestoreModalKeepsEveryDraft();
    void draftTimerStartsOnlyWithAutosave();
    void draftForReadOnlyFileGoesToNewTab();
};

void TestT111bDrafts::initTestCase()
{
    for (const char *name : {"HOME", "XDG_DATA_HOME", "XDG_CONFIG_HOME"}) {
        const QString value = qEnvironmentVariable(name);
        QVERIFY2(value.contains(kScratchMarker), qPrintable(QString("%1=%2").arg(name, value)));
    }
    QVERIFY2(cacheDir().contains(kScratchMarker), qPrintable(cacheDir()));
}

// Case 5: the cache cannot be written at quit; Cancel keeps the window open and
// restarts both timers that were running.
void TestT111bDrafts::quitCancelledWhenDraftsCannotBeStored()
{
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    typeUnsaved(window, QStringLiteral("unsaved"));
    window.setSessionAutosaveEnabled(true);
    UnwritableCache unwritable;
    QVERIFY(unwritable.ok);

    PromptAnswerer cancel(QMessageBox::Cancel);
    QVERIFY2(!window.close(), "the window closed although its drafts could not be stored");
    QCOMPARE(cancel.answered(), 1);
    QCOMPARE(cancel.missed(), 0);
    QVERIFY(cancel.texts().value(0).contains(QStringLiteral("not yet stored for recovery")));
    QVERIFY(window.isVisible());
    QVERIFY(window.sessionAutosaveActive());
    QTimer *timer = draftTimer(window);
    QVERIFY(timer);
    QVERIFY(timer->isActive());
}

// Case 5b: Cancel restarts only the timers active on entry - none before autosave.
void TestT111bDrafts::quitCancelledBeforeAutosaveLeavesTimersStopped()
{
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    typeUnsaved(window, QStringLiteral("unsaved"));
    UnwritableCache unwritable;
    QVERIFY(unwritable.ok);

    PromptAnswerer cancel(QMessageBox::Cancel);
    QVERIFY(!window.close());
    QCOMPARE(cancel.answered(), 1);
    QVERIFY(!window.sessionAutosaveActive());
    QTimer *timer = draftTimer(window);
    QVERIFY(timer);
    QVERIFY(!timer->isActive());
}

// Case 6: confirming the prompt quits.
void TestT111bDrafts::quitConfirmedDiscardsWhenDraftsCannotBeStored()
{
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    typeUnsaved(window, QStringLiteral("unsaved"));
    window.setSessionAutosaveEnabled(true);
    UnwritableCache unwritable;
    QVERIFY(unwritable.ok);

    PromptAnswerer quit(QStringLiteral("Quit and Discard Changes"));
    QVERIFY(window.close());
    QCOMPARE(quit.answered(), 1);
    QCOMPARE(quit.missed(), 0);
}

// Case 6b: nothing to store - no prompt at quit, no notification on a tick.
void TestT111bDrafts::nothingAtRiskQuitsWithoutPrompt()
{
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    window.setSessionAutosaveEnabled(true);
    UnwritableCache unwritable;
    QVERIFY(unwritable.ok);
    hideNotification(window);

    QTimer *timer = draftTimer(window);
    QVERIFY(timer);
    timer->start(20);
    QTest::qWait(150);
    QVERIFY2(!notificationVisible(window), "a tick with nothing to store notified");

    PromptAnswerer any(QMessageBox::Cancel);
    QVERIFY(window.close());
    QCOMPARE(any.answered() + any.missed(), 0);
}

// Spec 3.2.3: the first failed tick notifies; repeated failures do not, and a
// success re-arms the notification.
void TestT111bDrafts::periodicFailureNotifiesOncePerFailureRun()
{
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    typeUnsaved(window, QStringLiteral("unsaved"));
    window.setSessionAutosaveEnabled(true);
    hideNotification(window);
    QTimer *timer = draftTimer(window);
    QVERIFY(timer);
    {
        UnwritableCache unwritable;
        QVERIFY(unwritable.ok);
        timer->start(20);
        QTRY_VERIFY_WITH_TIMEOUT(notificationVisible(window), 1000);
        hideNotification(window);
        QTest::qWait(150);
        QVERIFY2(!notificationVisible(window), "a repeated failure notified again");
    }
    QTRY_COMPARE_WITH_TIMEOUT(cachedEntryCount(), 1, 1000);
    UnwritableCache again;
    QVERIFY(again.ok);
    QTRY_VERIFY_WITH_TIMEOUT(notificationVisible(window), 1000);
    PromptAnswerer quit(QStringLiteral("Quit and Discard Changes"));
    QVERIFY(window.close());
}

// Case 7: a save that runs inside a modal shown by restore (the "Could not open
// file" warning for a draft whose path is a directory) must not shrink the set.
void TestT111bDrafts::saveDuringRestoreModalKeepsEveryDraft()
{
    regression::TempDir dir;
    const QString folder = dir.filePath(QStringLiteral("a-folder"));
    QVERIFY(QDir().mkpath(folder));
    writeEntry(QStringLiteral("a.json"), draftEntry(QStringLiteral("draft one"), folder));
    writeEntry(QStringLiteral("b.json"), draftEntry(QStringLiteral("draft two"), QString()));
    writeEntry(QStringLiteral("c.json"), draftEntry(QStringLiteral("draft three"), QString()));

    MainWindow window;
    int savesInModal = 0;
    QTimer inModal;
    QObject::connect(&inModal, &QTimer::timeout, [&]() {
        auto *box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        if (!box || box->property("answered").toBool()) return;
        box->setProperty("answered", true);
        Session::instance().saveUnsavedDocuments(&window);
        ++savesInModal;
        box->close();
    });
    inModal.start(0);
    Session::instance().restoreDrafts(&window);
    inModal.stop();

    QCOMPARE(savesInModal, 1);
    for (const QString &text : {"draft one", "draft two", "draft three"})
        QVERIFY2(editorWithText(window, text), qPrintable(text));
    QCOMPARE(cachedEntryCount(), 3);
}

// Case 8: the draft timer exists, is idle after construction and runs at 30 s
// once autosave is enabled.
void TestT111bDrafts::draftTimerStartsOnlyWithAutosave()
{
    MainWindow window;
    QTimer *timer = draftTimer(window);
    QVERIFY2(timer, "no QTimer named draftBackupTimer");
    QVERIFY(!timer->isActive());
    window.setSessionAutosaveEnabled(true);
    QVERIFY(timer->isActive());
    QCOMPARE(timer->interval(), 30000);
    window.setSessionAutosaveEnabled(false);
    QVERIFY(!timer->isActive());
}

// Case 9: the file opens read-only (lossy decode); its draft goes to a new
// writable tab that keeps the path and saves the draft text.
void TestT111bDrafts::draftForReadOnlyFileGoesToNewTab()
{
    regression::TempDir dir;
    const QString path = dir.filePath(QStringLiteral("lone-surrogate.txt"));
    // A lossy decode as in test_t1_3_encoding lossyDecodeOpensReadOnly, but one
    // that open detects by itself: that fixture's bytes need a forced
    // ISO-8859-6 and auto-detect as lossless ISO-8859-1. Here the BOM fixes
    // UTF-16LE and the lone high surrogate (0xD800) cannot be re-encoded.
    const QByteArray bytes("\xFF\xFEh\0i\0\x00\xD8!\0\n\0", 12);
    QVERIFY(regression::writeBytes(path, bytes));
    const QString draftText = QStringLiteral("draft ascii text\n");
    QJsonObject draft = draftEntry(draftText, path);
    draft["lineEnding"] = QStringLiteral("LF");
    writeEntry(QStringLiteral("a.json"), draft);

    MainWindow window;
    Session::instance().restoreDrafts(&window);

    Editor *fileEditor = editorForFile(window, path);
    QVERIFY(fileEditor);
    QVERIFY(fileEditor->document()->isReadOnly());
    QVERIFY2(fileEditor->toPlainText() != draftText, "the draft landed in the read-only tab");
    QCOMPARE(fileEditor->toPlainText(), fileEditor->document()->text());

    Editor *draftEditor = editorWithText(window, draftText);
    QVERIFY2(draftEditor, "the draft text is in no tab");
    QVERIFY(draftEditor != fileEditor);
    QVERIFY(draftEditor->document()->isUntitled());
    QVERIFY(!draftEditor->document()->isReadOnly());
    QCOMPARE(draftEditor->document()->property("originalFilePath").toString(), path);

    const QString target = dir.filePath(QStringLiteral("saved-draft.txt"));
    QCOMPARE(draftEditor->document()->saveAs(target), SaveResult::Ok);
    QCOMPARE(regression::readBytes(target), draftText.toUtf8());
}

PRIMEEDIT_REGRESSION_MAIN(TestT111bDrafts)

#include "test_t1_11b_drafts.moc"

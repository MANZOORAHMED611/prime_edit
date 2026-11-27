// T3.1 regression tests (spec docs/superpowers/specs/2026-11-26-phase1-data-loss-fixes.md,
// D5 split view, section 9.2 row T3.1). Every test drives a real MainWindow
// through its split-view slots and the tab bars' close signals.
#include "support.h"

#include "core/document.h"
#include "core/documentmanager.h"
#include "ui/editor.h"
#include "ui/mainwindow.h"
#include "ui/tabwidget.h"
#include "utils/settings.h"

#include <QAbstractButton>
#include <QAction>
#include <QMessageBox>
#include <QPointer>
#include <QTabBar>
#include <QTimer>

namespace {

// Answers every QMessageBox that opens while it is alive with one button, so a
// close prompt can never block a test. Counts the prompts it answered.
class PromptAnswerer
{
public:
    explicit PromptAnswerer(QMessageBox::StandardButton answer)
    {
        QObject::connect(&m_timer, &QTimer::timeout, [this, answer]() {
            auto *box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if (!box || !box->button(answer)) return;
            ++m_answered;
            box->button(answer)->click();
        });
        m_timer.start(20);
    }
    int answered() const { return m_answered; }

private:
    QTimer m_timer;
    int m_answered = 0;
};

// The pane-2 tab widget: the TabWidget child that is not tabWidget().
TabWidget *secondPane(MainWindow &mw)
{
    for (TabWidget *tw : mw.findChildren<TabWidget*>()) {
        if (tw != mw.tabWidget()) return tw;
    }
    return nullptr;
}

QString plainLabel(const QAction *action)
{
    return action->text().remove(QLatin1Char('&'));
}

// Shows the window and waits until it is active, so focus moves are real.
bool showActive(MainWindow &mw)
{
    mw.show();
    mw.activateWindow();
    return QTest::qWaitForWindowActive(&mw);
}

bool moveCurrentToOtherView(MainWindow &mw)
{
    return QMetaObject::invokeMethod(&mw, "moveToOtherView", Qt::DirectConnection);
}

} // namespace

class TestT31Split : public QObject
{
    Q_OBJECT

private slots:
    void settingsChangeReachesPane2Editor();
    void closeFromTabBar1WithPane2FocusedClosesPane1Tab();
    void cloneIsAbsentAndSplitOpensEmptyPane();
    void movingModifiedUntitledToPane2AndClosingPane1DoesNotCrash();
};

// D5: the settings handlers reached only pane 1, so a pane-2 editor kept the
// old word-wrap and line-number state.
void TestT31Split::settingsChangeReachesPane2Editor()
{
    MainWindow mw;
    QVERIFY(showActive(mw));
    mw.newFile();
    Editor *moved = mw.currentEditor();
    QVERIFY(moved);
    QVERIFY(moveCurrentToOtherView(mw));
    TabWidget *pane2 = secondPane(mw);
    QVERIFY(pane2);
    QVERIFY(pane2->indexOf(moved) >= 0);

    Settings &settings = Settings::instance();
    const bool wrapBefore = settings.wordWrap();
    const bool numbersBefore = settings.showLineNumbers();
    settings.setWordWrap(!wrapBefore);
    settings.setShowLineNumbers(!numbersBefore);

    for (int i = 0; i < pane2->count(); ++i) {
        auto *e = qobject_cast<Editor*>(pane2->widget(i));
        QVERIFY(e);
        QCOMPARE(e->wordWrapEnabled(), !wrapBefore);
        QCOMPARE(e->lineNumbersVisible(), !numbersBefore);
    }
    settings.setWordWrap(wrapBefore);
    settings.setShowLineNumbers(numbersBefore);
}

// D5: a close request from tab bar 1 acted on the focused pane. With pane 2
// focused it closed a pane-2 tab, or nothing when the index did not exist there.
void TestT31Split::closeFromTabBar1WithPane2FocusedClosesPane1Tab()
{
    MainWindow mw;
    QVERIFY(showActive(mw));
    mw.newFile();
    mw.newFile();
    mw.newFile();
    QVERIFY(moveCurrentToOtherView(mw));
    TabWidget *pane1 = mw.tabWidget();
    TabWidget *pane2 = secondPane(mw);
    QVERIFY(pane2);
    QCOMPARE(pane1->count(), 3);

    QWidget *pane2Editor = pane2->widget(pane2->count() - 1);
    pane2Editor->setFocus();
    QTRY_VERIFY(pane2Editor->hasFocus());

    QPointer<QWidget> target = pane1->widget(2);
    const int pane2CountBefore = pane2->count();
    emit pane1->tabBar()->tabCloseRequested(2);

    QCOMPARE(pane1->count(), 2);
    QCOMPARE(pane1->indexOf(target), -1);
    QCOMPARE(pane2->count(), pane2CountBefore);
}

// D5: Clone made an independent copy whose save overwrote the other view.
// It is removed, and splitting no longer clones the current document.
void TestT31Split::cloneIsAbsentAndSplitOpensEmptyPane()
{
    MainWindow mw;
    QVERIFY(showActive(mw));

    for (const QAction *action : mw.findChildren<QAction*>()) {
        QVERIFY2(plainLabel(action).compare(QStringLiteral("Clone to Other View"), Qt::CaseInsensitive) != 0,
                 "the Clone to Other View action still exists");
    }
    QCOMPARE(MainWindow::staticMetaObject.indexOfMethod("cloneToOtherView()"), -1);

    const int editorsBefore = mw.allEditors().size();
    QVERIFY(QMetaObject::invokeMethod(&mw, "splitVertical", Qt::DirectConnection));
    QVERIFY(mw.isSplit());
    TabWidget *pane2 = secondPane(mw);
    QVERIFY(pane2);
    QCOMPARE(pane2->count(), 0);
    QCOMPARE(mw.allEditors().size(), editorsBefore);
}

// D5: closing a tab could free a Document another editor still showed. After
// moving a modified untitled document to pane 2 and closing every pane-1 tab
// from tab bar 1, the pane-2 editor's Document must still be alive and intact.
void TestT31Split::movingModifiedUntitledToPane2AndClosingPane1DoesNotCrash()
{
    MainWindow mw;
    QVERIFY(showActive(mw));
    mw.newFile();
    Editor *draft = mw.currentEditor();
    QVERIFY(draft);
    const QString text = QStringLiteral("unsaved draft text");
    draft->insertPlainText(text);
    QVERIFY(draft->document()->isModified());
    QVERIFY(moveCurrentToOtherView(mw));

    TabWidget *pane1 = mw.tabWidget();
    TabWidget *pane2 = secondPane(mw);
    QVERIFY(pane2);
    draft->setFocus();
    QTRY_VERIFY(draft->hasFocus());

    PromptAnswerer discard(QMessageBox::Discard);
    for (int n = pane1->count(); n > 0; --n) {
        emit pane1->tabBar()->tabCloseRequested(0);
    }
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    // Liveness first: on failure nothing below may dereference a freed Document.
    QVERIFY(DocumentManager::instance().documents().contains(draft->document()));
    QCOMPARE(discard.answered(), 0);
    QVERIFY(mw.isSplit());
    QCOMPARE(pane2->count(), 1);
    QCOMPARE(pane2->widget(0), static_cast<QWidget*>(draft));
    QVERIFY(draft->document()->isModified());
    QCOMPARE(draft->toPlainText(), text);
    QCOMPARE(pane1->count(), 1);
}

PRIMEEDIT_REGRESSION_MAIN(TestT31Split)

#include "test_t3_1_split.moc"

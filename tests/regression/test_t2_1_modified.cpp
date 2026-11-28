// T2.1 (spec D1, editor side): the 2 s RTL pass must not leave the QTextDocument
// flag set behind the Document's back. In 1.0.0 it did, so the next keystroke
// raised no modificationChanged and the edit was never flagged: Save stayed
// disabled and the unsaved cache skipped the document.
#include "support.h"

#include "core/document.h"
#include "core/session.h"
#include "ui/editor.h"
#include "ui/mainwindow.h"

#include <QAction>
#include <QDir>
#include <QStandardPaths>
#include <QTextBlock>

namespace {

// The RTL timer is 2000 ms; wait past it so the pass has run.
constexpr int kAfterRtlPassMs = 2500;

QString unsavedDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/sessions/unsaved");
}

QAction *saveAction(MainWindow &window)
{
    const QList<QAction *> actions = window.findChildren<QAction *>();
    for (QAction *action : actions) {
        if (action->text() == MainWindow::tr("&Save"))
            return action;
    }
    return nullptr;
}

bool cacheContains(const QByteArray &needle)
{
    QDir dir(unsavedDir());
    const QStringList files = dir.entryList({QStringLiteral("*.json")}, QDir::Files);
    for (const QString &name : files) {
        if (regression::readBytes(dir.filePath(name)).contains(needle))
            return true;
    }
    return false;
}

} // namespace

class TestT21Modified : public QObject
{
    Q_OBJECT

private:
    // Opens text in a MainWindow, lets the RTL pass run, types one character,
    // and checks every consumer of the modified flag.
    void checkEditAfterRtlPass(const QByteArray &utf8, const QByteArray &marker)
    {
        regression::TempDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("doc.txt"));
        QVERIFY(regression::writeBytes(path, utf8));

        MainWindow window;
        window.show();
        Editor *editor = window.openFile(path);
        QVERIFY(editor);
        QVERIFY(!editor->document()->isModified());

        QTest::qWait(kAfterRtlPassMs);
        QVERIFY2(!editor->document()->isModified(), "the RTL pass alone must not mark the file modified");

        editor->goToLine(2);
        QTest::keyClicks(editor, QStringLiteral("Z"));

        QVERIFY2(editor->document()->isModified(), "a keystroke after the RTL pass is not flagged modified");
        QAction *save = saveAction(window);
        QVERIFY(save);
        QVERIFY2(save->isEnabled(), "Save stays disabled after an edit");

        Session::instance().saveUnsavedDocuments(&window);
        QVERIFY2(cacheContains(marker), "the edited document was not written to the unsaved cache");
    }

private slots:
    void init() { QDir(unsavedDir()).removeRecursively(); }
    void cleanup() { QDir(unsavedDir()).removeRecursively(); }

    void editAfterRtlPassIsModified_latin()
    {
        checkEditAfterRtlPass("plain latin line\nsecond line\n", "Zsecond line");
    }

    void editAfterRtlPassIsModified_arabic()
    {
        const QByteArray text = QStringLiteral("مرحبا بالعالم\n"
                                               "latin line\n").toUtf8();
        checkEditAfterRtlPass(text, "Zlatin line");
    }

    // Spec D1: the pass changes a block's format only for Arabic or previously
    // RTL blocks, so Latin blocks keep LayoutDirectionAuto and no undo entries
    // are created for them.
    void rtlPassLeavesLatinBlocksAlone()
    {
        Document doc;
        doc.setText(QStringLiteral("alpha\nbeta\n"));
        doc.setModified(false);
        Editor editor(&doc);
        editor.show();

        QTest::qWait(kAfterRtlPassMs);
        for (QTextBlock b = editor.QPlainTextEdit::document()->begin(); b.isValid(); b = b.next()) {
            QCOMPARE(b.blockFormat().layoutDirection(), Qt::LayoutDirectionAuto);
        }
        QVERIFY(!editor.QPlainTextEdit::document()->isUndoAvailable());
    }
};

PRIMEEDIT_REGRESSION_MAIN(TestT21Modified)

#include "test_t2_1_modified.moc"

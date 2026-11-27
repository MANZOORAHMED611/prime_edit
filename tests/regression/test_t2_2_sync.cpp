// T2.2 (spec D7 editor side, D11): syncToDocument copies rawText(), so U+00A0
// and U+2028 reach the file unchanged, and it no longer clears the editor's
// modified flag before the save is attempted, so a failed save leaves the
// document modified.
#include "support.h"

#include "core/document.h"
#include "ui/editor.h"
#include "ui/mainwindow.h"

#include <QTextDocument>

namespace {

QTextDocument *textDocument(Editor *editor)
{
    return editor->QPlainTextEdit::document();
}

} // namespace

class TestT22Sync : public QObject
{
    Q_OBJECT

private:
    // Open, type one character, delete it, save through MainWindow::saveFile:
    // the bytes on disk must be the bytes that were opened (spec I1, D11).
    void checkTypeDeleteSaveRoundTrip(const QByteArray &bytes)
    {
        regression::TempDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("doc.txt"));
        QVERIFY(regression::writeBytes(path, bytes));

        MainWindow window;
        window.show();
        Editor *editor = window.openFile(path);
        QVERIFY(editor);

        editor->moveCursor(QTextCursor::Start);
        QTest::keyClicks(editor, QStringLiteral("x"));
        QTest::keyClick(editor, Qt::Key_Backspace);
        QVERIFY(editor->document()->isModified());

        window.saveFile();

        QCOMPARE(regression::readBytes(path), bytes);
        QVERIFY(!editor->document()->isModified());
        QVERIFY(!textDocument(editor)->isModified());
    }

private slots:
    void noBreakSpaceSurvivesTypeDeleteSave()
    {
        checkTypeDeleteSaveRoundTrip(QStringLiteral("a b  c\nnext line\n").toUtf8());
    }

    void lineSeparatorSurvivesTypeDeleteSave()
    {
        checkTypeDeleteSaveRoundTrip(QStringLiteral("first second\nthird \n").toUtf8());
    }

    // MainWindow::saveFile calls syncToDocument before the save and clears the
    // flags only on SaveResult::Ok. A failed save must leave both flags set, and
    // the clean point must stay at the last real save: typing then undoing after
    // the failure must not make the document look clean.
    void failedSaveLeavesEditorModified()
    {
        regression::TempDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("doc.txt"));
        QVERIFY(regression::writeBytes(path, "original\n"));

        Document doc;
        QVERIFY(doc.load(path));
        Editor editor(&doc);
        editor.show();

        editor.moveCursor(QTextCursor::End);
        QTest::keyClicks(&editor, QStringLiteral("x"));
        QVERIFY(doc.isModified());

        // A directory that does not exist fails for every user, including root.
        editor.syncToDocument();
        const QString unwritable = tmp.filePath(QStringLiteral("missing-dir/doc.txt"));
        QVERIFY(doc.saveAs(unwritable) != SaveResult::Ok);

        QVERIFY2(textDocument(&editor)->isModified(), "syncToDocument cleared the editor flag before the save");
        QVERIFY(doc.isModified());

        // Typed away from "x", so Qt keeps it a separate undo step instead of
        // merging the two insertions; undo then leaves "x" in place.
        editor.moveCursor(QTextCursor::Start);
        QTest::keyClicks(&editor, QStringLiteral("y"));
        textDocument(&editor)->undo();
        QCOMPARE(editor.rawText(), QStringLiteral("original\nx"));
        QVERIFY2(textDocument(&editor)->isModified(), "undo after a failed save made the editor look clean");
        QVERIFY2(doc.isModified(), "undo after a failed save made the document look clean");
        QCOMPARE(regression::readBytes(path), QByteArray("original\n"));
    }
};

PRIMEEDIT_REGRESSION_MAIN(TestT22Sync)

#include "test_t2_2_sync.moc"

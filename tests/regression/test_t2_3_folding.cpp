// T2.3 (spec D2): folding is view-only. A fold hides its body blocks with
// QTextBlock::setVisible(false) and never removes or inserts text, so Save
// writes the full file. Any cursor, selection, Find, Go to Line or multi-cursor
// that reaches hidden text unfolds it first; removing a header shows its body;
// undo that re-creates hidden text leaves it visible.
#include "support.h"

#include "core/document.h"
#include "core/session.h"
#include "ui/editor.h"
#include "ui/mainwindow.h"

#include <QTextBlock>
#include <QTextDocument>

namespace {

// Block 0 is the header; blocks 1-3 are its body; block 4 follows the fold.
const QString kSource = QStringLiteral("int foo() {\n  foo();\n  bar();\n}\nend\n");

QTextDocument *textDocument(Editor *editor)
{
    return editor->QPlainTextEdit::document();
}

QTextBlock blockAt(Editor *editor, int number)
{
    return textDocument(editor)->findBlockByNumber(number);
}

bool allBlocksVisible(Editor *editor)
{
    for (QTextBlock b = textDocument(editor)->begin(); b.isValid(); b = b.next()) {
        if (!b.isVisible())
            return false;
    }
    return true;
}

// The document's text is the opened text: the fold changed only the view.
void bindSource(Document &doc, const QString &text)
{
    doc.setText(text);
    doc.setLanguage(QStringLiteral("cpp"));
    doc.setModified(false);
}

} // namespace

class TestT23Folding : public QObject
{
    Q_OBJECT

private slots:
    void foldIsViewOnly()
    {
        Document doc;
        bindSource(doc, kSource);
        Editor editor(&doc);
        editor.show();

        editor.foldAt(0);

        QVERIFY(editor.isFoldedLine(0));
        QCOMPARE(editor.rawText(), kSource);
        QVERIFY(blockAt(&editor, 0).isVisible());
        QVERIFY(!blockAt(&editor, 1).isVisible());
        QVERIFY(!blockAt(&editor, 3).isVisible());
        QVERIFY(blockAt(&editor, 4).isVisible());
        QVERIFY2(!textDocument(&editor)->isUndoAvailable(), "folding created an undo entry");
        QVERIFY(!textDocument(&editor)->isModified());
        QVERIFY(!doc.isModified());

        editor.unfoldAt(0);
        QVERIFY(!editor.isFoldedLine(0));
        QVERIFY(allBlocksVisible(&editor));
        QCOMPARE(editor.rawText(), kSource);
    }

    void foldTypeSaveKeepsFullText()
    {
        regression::TempDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("fold.cpp"));
        QVERIFY(regression::writeBytes(path, kSource.toUtf8()));

        MainWindow window;
        window.show();
        Editor *editor = window.openFile(path);
        QVERIFY(editor);
        editor->foldAt(0);
        QVERIFY(editor->isFoldedLine(0));

        editor->moveCursor(QTextCursor::End);
        QTest::keyClicks(editor, QStringLiteral("x"));
        window.saveFile();

        QCOMPARE(regression::readBytes(path), (kSource + QLatin1Char('x')).toUtf8());
        QVERIFY2(editor->isFoldedLine(0), "typing outside the fold must not unfold it");
    }

    void findInsideFoldUnfoldsBeforeReplace()
    {
        Document doc;
        bindSource(doc, kSource);
        Editor editor(&doc);
        editor.show();
        editor.foldAt(0);

        QVERIFY(editor.findNext(QStringLiteral("bar()")));
        QVERIFY2(blockAt(&editor, 2).isVisible(), "the Find match is still hidden");
        QVERIFY(!editor.isFoldedLine(0));
        editor.replaceSelection(QStringLiteral("baz()"));
        QCOMPARE(editor.rawText(), QString(kSource).replace(QStringLiteral("bar()"), QStringLiteral("baz()")));
    }

    void replaceAllInsideFoldUnfolds()
    {
        Document doc;
        bindSource(doc, kSource);
        Editor editor(&doc);
        editor.show();
        editor.foldAt(0);

        QCOMPARE(editor.replaceAll(QStringLiteral("bar()"), QStringLiteral("baz()")), 1);
        QVERIFY2(blockAt(&editor, 2).isVisible(), "Replace All edited hidden text");
        QCOMPARE(editor.rawText(), QString(kSource).replace(QStringLiteral("bar()"), QStringLiteral("baz()")));
    }

    void goToLineInsideFoldUnfolds()
    {
        Document doc;
        bindSource(doc, kSource);
        Editor editor(&doc);
        editor.show();
        editor.foldAt(0);

        editor.goToLine(3);
        QCOMPARE(editor.textCursor().block().text(), QStringLiteral("  bar();"));
        QVERIFY2(editor.textCursor().block().isVisible(), "Go to Line left the cursor in hidden text");
        QVERIFY(!editor.isFoldedLine(0));
    }

    // A programmatic QTextCursor::Down does land in a hidden block.
    void cursorEnteringHiddenBlockUnfolds()
    {
        Document doc;
        bindSource(doc, kSource);
        Editor editor(&doc);
        editor.show();
        editor.foldAt(0);

        QTextCursor cursor(blockAt(&editor, 0));
        cursor.movePosition(QTextCursor::Down);
        editor.setTextCursor(cursor);
        QCOMPARE(editor.textCursor().block().text(), QStringLiteral("  foo();"));
        QVERIFY(editor.textCursor().block().isVisible());
        QVERIFY(allBlocksVisible(&editor));
    }

    void multiCursorInsideFoldUnfoldsBeforeEdit()
    {
        Document doc;
        bindSource(doc, kSource);
        Editor editor(&doc);
        editor.show();
        editor.foldAt(0);

        QTextCursor header(blockAt(&editor, 0));
        header.setPosition(4);
        header.setPosition(7, QTextCursor::KeepAnchor); // "foo" on the header line
        editor.setTextCursor(header);
        editor.selectAllOccurrences();
        QVERIFY(editor.hasMultipleCursors());
        QVERIFY2(blockAt(&editor, 1).isVisible(), "a multi-cursor sits in hidden text");

        QTest::keyClicks(&editor, QStringLiteral("q"));
        QCOMPARE(editor.rawText(), QStringLiteral("int q() {\n  q();\n  bar();\n}\nend\n"));
    }

    void backspaceRemovingHeaderShowsBody()
    {
        Document doc;
        bindSource(doc, QStringLiteral("start\n") + kSource);
        Editor editor(&doc);
        editor.show();
        editor.foldAt(1);
        QVERIFY(editor.isFoldedLine(1));

        editor.setTextCursor(QTextCursor(blockAt(&editor, 1)));
        QTest::keyClick(&editor, Qt::Key_Backspace); // joins the header onto "start"

        QCOMPARE(editor.rawText(), QStringLiteral("start") + kSource);
        QVERIFY2(allBlocksVisible(&editor), "the body of a removed header is still hidden");
    }

    void deleteLineRemovingHeaderShowsBody()
    {
        Document doc;
        bindSource(doc, QStringLiteral("start\n") + kSource);
        Editor editor(&doc);
        editor.show();
        editor.foldAt(1);

        editor.setTextCursor(QTextCursor(blockAt(&editor, 1)));
        editor.deleteLine();

        QCOMPARE(editor.rawText(), QStringLiteral("start\n  foo();\n  bar();\n}\nend\n"));
        QVERIFY2(allBlocksVisible(&editor), "the body of a deleted header is still hidden");
    }

    // A command that edits through its own QTextCursor bypasses the cursor
    // rule. Undoing its removal re-creates the hidden text, which must be
    // visible and no longer folded.
    void undoRecreatingHiddenTextLeavesItVisible()
    {
        Document doc;
        bindSource(doc, kSource);
        Editor editor(&doc);
        editor.show();
        editor.foldAt(0);

        const QTextBlock header = blockAt(&editor, 0);
        const QTextBlock last = blockAt(&editor, 3);
        QTextCursor remover(textDocument(&editor));
        remover.setPosition(header.position() + header.length() - 1);
        remover.setPosition(last.position() + last.length() - 1, QTextCursor::KeepAnchor);
        remover.removeSelectedText();
        QCOMPARE(editor.rawText(), QStringLiteral("int foo() {\nend\n"));

        textDocument(&editor)->undo();
        QCOMPARE(editor.rawText(), kSource);
        QVERIFY2(allBlocksVisible(&editor), "undo re-created hidden text");
        QVERIFY(!editor.isFoldedLine(0));
    }

    // Session contract: foldedRegions() is the list of header block numbers,
    // and session restore re-applies each with foldAt().
    void sessionRoundTripKeepsFoldsAndText()
    {
        regression::TempDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("session.cpp"));
        const QString source = kSource + QStringLiteral("void g() {\n  g();\n}\n");
        QVERIFY(regression::writeBytes(path, source.toUtf8()));

        {
            MainWindow first;
            first.show();
            Editor *editor = first.openFile(path);
            QVERIFY(editor);
            editor->foldAt(0);
            editor->foldAt(5);
            QCOMPARE(editor->foldedRegions(), QList<int>({0, 5}));
            Session::instance().saveSession(QStringLiteral("t2_3_folds"), &first);
        }

        MainWindow second;
        second.show();
        Session::instance().loadSession(QStringLiteral("t2_3_folds"), &second);
        Editor *restored = nullptr;
        for (Editor *e : second.allEditors()) {
            if (e->document() && e->document()->filePath() == path)
                restored = e;
        }
        QVERIFY(restored);
        QCOMPARE(restored->foldedRegions(), QList<int>({0, 5}));
        QVERIFY(!blockAt(restored, 1).isVisible());
        QVERIFY(!blockAt(restored, 6).isVisible());
        QCOMPARE(restored->rawText(), source);
    }
};

PRIMEEDIT_REGRESSION_MAIN(TestT23Folding)

#include "test_t2_3_folding.moc"

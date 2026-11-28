// Notepad++ parity quick wins, batch 1. Covers every pure function in
// TextTransforms and CodecTransforms, and checks at widget level that a
// transform applied in the Editor is one undo step and marks the document
// modified.
#include "support.h"

#include "core/document.h"
#include "core/texttransforms.h"
#include "ui/editor.h"

#include <QTextDocument>

using namespace TextTransforms;

namespace {

const QString kArabic = QStringLiteral("مرحبا");  // "marhaban"

// The QTextDocument behind an Editor (Editor::document() is the Document).
QTextDocument *textDoc(Editor &editor)
{
    return static_cast<QPlainTextEdit &>(editor).document();
}

} // namespace

class TestNppQw1 : public QObject
{
    Q_OBJECT

private slots:
    // ---- splitting and joining -------------------------------------------
    void splitEmpty()
    {
        const SplitText s = splitLines(QString());
        QCOMPARE(s.lines, QStringList{QString()});
        QVERIFY(!s.trailingEol);
        QCOMPARE(joinLines(s), QString());
    }
    void splitKeepsTrailingEol()
    {
        const SplitText s = splitLines(QStringLiteral("a\nb\n"));
        QCOMPARE(s.lines, (QStringList{"a", "b"}));
        QVERIFY(s.trailingEol);
        QCOMPARE(joinLines(s), QStringLiteral("a\nb\n"));
    }
    void splitCrlf()
    {
        const SplitText s = splitLines(QStringLiteral("a\r\nb"));
        QCOMPARE(s.lines, (QStringList{"a", "b"}));
        QCOMPARE(s.eol, QStringLiteral("\r\n"));
        QCOMPARE(joinLines(s), QStringLiteral("a\r\nb"));
    }
    void splitMixedUsesFirstEol()
    {
        const SplitText s = splitLines(QStringLiteral("a\rb\nc\r\nd"));
        QCOMPARE(s.lines, (QStringList{"a", "b", "c", "d"}));
        QCOMPARE(joinLines(s), QStringLiteral("a\rb\rc\rd"));
    }
    void splitOnlyEol()
    {
        const SplitText s = splitLines(QStringLiteral("\n"));
        QCOMPARE(s.lines, QStringList{QString()});
        QVERIFY(s.trailingEol);
        QCOMPARE(joinLines(s), QStringLiteral("\n"));
    }

    // ---- group 1: line and blank operations ------------------------------
    void removeEmptyLines_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<QString>("out");
        QTest::newRow("empty") << "" << "";
        QTest::newRow("keeps blanks") << "a\n\n \nb" << "a\n \nb";
        QTest::newRow("trailing eol kept") << "a\n\nb\n" << "a\nb\n";
        QTest::newRow("crlf") << "a\r\n\r\nb" << "a\r\nb";
        QTest::newRow("all empty") << "\n\n" << "\n";
        QTest::newRow("arabic") << kArabic + "\n\n" + kArabic << kArabic + "\n" + kArabic;
    }
    void removeEmptyLines()
    {
        QFETCH(QString, in);
        QFETCH(QString, out);
        QCOMPARE(TextTransforms::removeEmptyLines(in), out);
    }
    void removeBlankLines_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<QString>("out");
        QTest::newRow("empty") << "" << "";
        QTest::newRow("blanks") << "a\n\n \t\nb" << "a\nb";
        QTest::newRow("no trailing eol") << "a\n  " << "a";
        QTest::newRow("mixed eol") << "a\r\n \nb\rc" << "a\r\nb\r\nc";
    }
    void removeBlankLines()
    {
        QFETCH(QString, in);
        QFETCH(QString, out);
        QCOMPARE(TextTransforms::removeBlankLines(in), out);
    }
    void removeConsecutiveDuplicates_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<QString>("out");
        QTest::newRow("empty") << "" << "";
        QTest::newRow("runs") << "a\na\nb\na\na" << "a\nb\na";
        QTest::newRow("case matters") << "a\nA" << "a\nA";
        QTest::newRow("trailing eol") << "x\nx\n" << "x\n";
        QTest::newRow("empty runs") << "\n\n\nz" << "\nz";
    }
    void removeConsecutiveDuplicates()
    {
        QFETCH(QString, in);
        QFETCH(QString, out);
        QCOMPARE(removeConsecutiveDuplicateLines(in), out);
    }
    void trims_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<QString>("leading");
        QTest::addColumn<QString>("trailing");
        QTest::addColumn<QString>("both");
        QTest::newRow("empty") << "" << "" << "" << "";
        QTest::newRow("lines") << "  a \n\tb\t\n" << "a \nb\t\n" << "  a\n\tb\n" << "a\nb\n";
        QTest::newRow("crlf") << " a \r\n b " << "a \r\nb " << " a\r\n b" << "a\r\nb";
        QTest::newRow("nbsp kept") << QString(" a") << QString(" a")
                                   << QString(" a") << QString(" a");
    }
    void trims()
    {
        QFETCH(QString, in);
        QFETCH(QString, leading);
        QFETCH(QString, trailing);
        QFETCH(QString, both);
        QCOMPARE(trimLeading(in), leading);
        QCOMPARE(trimTrailing(in), trailing);
        QCOMPARE(trimBoth(in), both);
    }
    void eolToSpace_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<QString>("plain");
        QTest::addColumn<QString>("trimmed");
        QTest::newRow("empty") << "" << "" << "";
        QTest::newRow("lines") << "a\nb\nc" << "a b c" << "a b c";
        QTest::newRow("trailing eol kept") << " a \n b \n" << " a   b \n" << "a b\n";
        QTest::newRow("crlf") << "a\r\nb" << "a b" << "a b";
        QTest::newRow("mixed") << "a\rb\nc" << "a b c" << "a b c";
    }
    void eolToSpace()
    {
        QFETCH(QString, in);
        QFETCH(QString, plain);
        QFETCH(QString, trimmed);
        QCOMPARE(TextTransforms::eolToSpace(in), plain);
        QCOMPARE(trimBothAndEolToSpace(in), trimmed);
    }

    // ---- group 2: case -----------------------------------------------------
    void properCase_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<QString>("force");
        QTest::addColumn<QString>("blend");
        QTest::newRow("empty") << "" << "" << "";
        QTest::newRow("words") << "hello wORLD" << "Hello World" << "Hello WORLD";
        QTest::newRow("apostrophe") << "don't STOP" << "Don't Stop" << "Don't STOP";
        QTest::newRow("digit prefix") << "3rd place" << "3rd Place" << "3rd Place";
        QTest::newRow("separators") << "foo_bar-baz" << "Foo_Bar-Baz" << "Foo_Bar-Baz";
        QTest::newRow("lines crlf") << "a b\r\nc d" << "A B\r\nC D" << "A B\r\nC D";
        QTest::newRow("accents") << QString("élan ÉCOLE") << QString("Élan École")
                                 << QString("Élan ÉCOLE");
        QTest::newRow("arabic") << kArabic << kArabic << kArabic;
        // U+10428 DESERET SMALL LETTER LONG I uppercases to U+10400 (surrogate pairs)
        QTest::newRow("non-bmp") << QString::fromUcs4(U"\U00010428\U00010428", 2)
                                 << QString::fromUcs4(U"\U00010400\U00010428", 2)
                                 << QString::fromUcs4(U"\U00010400\U00010428", 2);
    }
    void properCase()
    {
        QFETCH(QString, in);
        QFETCH(QString, force);
        QFETCH(QString, blend);
        QCOMPARE(TextTransforms::properCase(in, false), force);
        QCOMPARE(TextTransforms::properCase(in, true), blend);
    }
    void sentenceCase_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<QString>("force");
        QTest::addColumn<QString>("blend");
        QTest::newRow("empty") << "" << "" << "";
        QTest::newRow("punctuation") << "hello WORLD. this IS it! ok? yes"
                                     << "Hello world. This is it! Ok? Yes"
                                     << "Hello WORLD. This IS it! Ok? Yes";
        QTest::newRow("single newline continues") << "one\ntwo" << "One\ntwo" << "One\ntwo";
        QTest::newRow("blank line starts") << "one\n\ntwo" << "One\n\nTwo" << "One\n\nTwo";
        QTest::newRow("crlf blank line") << "one\r\n\r\ntwo" << "One\r\n\r\nTwo" << "One\r\n\r\nTwo";
        QTest::newRow("cr blank line") << "one\r\rtwo" << "One\r\rTwo" << "One\r\rTwo";
        QTest::newRow("digit first") << "3 apples. ok" << "3 Apples. Ok" << "3 Apples. Ok";
        QTest::newRow("arabic") << kArabic + ". " + kArabic << kArabic + ". " + kArabic
                                << kArabic + ". " + kArabic;
    }
    void sentenceCase()
    {
        QFETCH(QString, in);
        QFETCH(QString, force);
        QFETCH(QString, blend);
        QCOMPARE(TextTransforms::sentenceCase(in, false), force);
        QCOMPARE(TextTransforms::sentenceCase(in, true), blend);
    }
    void invertCase()
    {
        QCOMPARE(TextTransforms::invertCase(QString()), QString());
        QCOMPARE(TextTransforms::invertCase(QStringLiteral("Hello, World 42")),
                 QStringLiteral("hELLO, wORLD 42"));
        QCOMPARE(TextTransforms::invertCase(kArabic), kArabic);
        QCOMPARE(TextTransforms::invertCase(QString::fromUcs4(U"\U00010400", 1)),
                 QString::fromUcs4(U"\U00010428", 1));
    }
    void randomCase()
    {
        const QString in = QStringLiteral("The quick brown fox, 123!\r\n") + kArabic;
        const QString out = TextTransforms::randomCase(in, 7);
        QCOMPARE(out.size(), in.size());
        QCOMPARE(out.toLower(), in.toLower());
        QVERIFY(out.endsWith(QStringLiteral(", 123!\r\n") + kArabic));
        QCOMPARE(TextTransforms::randomCase(in, 7), out);  // deterministic per seed
        QCOMPARE(TextTransforms::randomCase(QString(), 7), QString());
    }

    // ---- editor adapter ---------------------------------------------------
    // A whole-document transform is one undo step and marks the file modified.
    void editorTransformIsOneUndoStep()
    {
        Document doc;
        Editor editor(&doc);
        editor.setPlainText(QStringLiteral("a\n\nb\n\nc"));
        textDoc(editor)->setModified(false);
        doc.setModified(false);

        QVERIFY(editor.applyTextTransform(TransformScope::LinesOrDocument,
                                          TextTransforms::removeEmptyLines));
        QCOMPARE(editor.toPlainText(), QStringLiteral("a\nb\nc"));
        QVERIFY(doc.isModified());
        QVERIFY(textDoc(editor)->isModified());

        editor.undo();
        QCOMPARE(editor.toPlainText(), QStringLiteral("a\n\nb\n\nc"));
        QVERIFY(!textDoc(editor)->isUndoAvailable());
    }
    // With a selection, line scope covers whole lines and ignores a final
    // line the selection only touches at column 0.
    void editorLineScopeUsesSelectedLines()
    {
        Document doc;
        Editor editor(&doc);
        editor.setPlainText(QStringLiteral("  a\n  b\n  c\n  d"));
        QTextCursor cursor = editor.textCursor();
        cursor.setPosition(5);   // inside line 2
        cursor.setPosition(12, QTextCursor::KeepAnchor);  // start of line 4
        editor.setTextCursor(cursor);
        QVERIFY(editor.applyTextTransform(TransformScope::LinesOrDocument, trimLeading));
        QCOMPARE(editor.toPlainText(), QStringLiteral("  a\nb\nc\n  d"));
    }
    void editorNoChangeIsNotAnEdit()
    {
        Document doc;
        Editor editor(&doc);
        editor.setPlainText(QStringLiteral("a\nb"));
        textDoc(editor)->setModified(false);
        doc.setModified(false);
        QVERIFY(!editor.applyTextTransform(TransformScope::LinesOrDocument,
                                           TextTransforms::removeEmptyLines));
        QVERIFY(!doc.isModified());
    }
    void editorReadOnlyRefuses()
    {
        Document doc;
        Editor editor(&doc);
        editor.setPlainText(QStringLiteral("a\n\nb"));
        doc.setReadOnly(true);
        QVERIFY(!editor.applyTextTransform(TransformScope::LinesOrDocument,
                                           TextTransforms::removeEmptyLines));
        QCOMPARE(editor.toPlainText(), QStringLiteral("a\n\nb"));
    }
    void editorSelectionScopeNeedsSelection()
    {
        Document doc;
        Editor editor(&doc);
        editor.setPlainText(QStringLiteral(" a "));
        QVERIFY(!editor.applyTextTransform(TransformScope::Selection, trimBoth));
        QCOMPARE(editor.toPlainText(), QStringLiteral(" a "));
    }
};

PRIMEEDIT_REGRESSION_MAIN(TestNppQw1)
#include "test_npp_qw1.moc"

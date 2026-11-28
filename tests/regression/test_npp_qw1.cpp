// Notepad++ parity quick wins, batch 1. Covers every pure function in
// TextTransforms and CodecTransforms, and checks at widget level that a
// transform applied in the Editor is one undo step and marks the document
// modified.
#include "support.h"

#include "core/charactersets.h"
#include "core/codectransforms.h"
#include "core/commandline.h"
#include "core/document.h"
#include "core/singleinstance.h"
#include "core/textcodec.h"
#include "syntax/languagemanager.h"
#include "core/texttransforms.h"
#include "ui/editor.h"
#include "ui/mainwindow.h"
#include "ui/searchdialog.h"

#include <QAction>
#include <QClipboard>
#include <QCommandLineParser>
#include <QDialog>
#include <QDir>
#include <QPlainTextEdit>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>

#include <atomic>
#include <thread>

using namespace TextTransforms;
using namespace CodecTransforms;

namespace {

const QString kArabic = QStringLiteral("مرحبا");  // "marhaban"

// The QTextDocument behind an Editor (Editor::document() is the Document).
QTextDocument *textDoc(Editor &editor)
{
    return static_cast<QPlainTextEdit &>(editor).document();
}

// The Find dialog, created the way Search > Find creates it.
SearchDialog *searchDialog(MainWindow &mw)
{
    QMetaObject::invokeMethod(&mw, "find", Qt::DirectConnection);
    return mw.findChild<SearchDialog *>();
}

// Triggers the menu action with this exact text; false when there is none.
bool trigger(MainWindow &mw, const QString &text)
{
    for (QAction *action : mw.findChildren<QAction *>()) {
        if (action->text() == text) {
            action->trigger();
            return true;
        }
    }
    return false;
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

    // ---- group 3: sorting and line order ---------------------------------
    void sortLexicographic_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<int>("cs");
        QTest::addColumn<QString>("asc");
        QTest::addColumn<QString>("desc");
        QTest::newRow("empty") << "" << int(Qt::CaseSensitive) << "" << "";
        QTest::newRow("case sensitive") << "b\na\nC" << int(Qt::CaseSensitive) << "C\na\nb" << "b\na\nC";
        QTest::newRow("ignoring case") << "b\na\nC" << int(Qt::CaseInsensitive) << "a\nb\nC" << "C\nb\na";
        QTest::newRow("stable ties") << "B\nb\nA" << int(Qt::CaseInsensitive) << "A\nB\nb" << "B\nb\nA";
        QTest::newRow("trailing eol stays last") << "b\na\n" << int(Qt::CaseSensitive) << "a\nb\n" << "b\na\n";
        QTest::newRow("no trailing eol") << "b\n\na" << int(Qt::CaseSensitive) << "\na\nb" << "b\na\n";
        QTest::newRow("crlf") << "b\r\na" << int(Qt::CaseSensitive) << "a\r\nb" << "b\r\na";
    }
    void sortLexicographic()
    {
        QFETCH(QString, in);
        QFETCH(int, cs);
        QFETCH(QString, asc);
        QFETCH(QString, desc);
        const auto sensitivity = Qt::CaseSensitivity(cs);
        QCOMPARE(TextTransforms::sortLexicographic(in, SortOrder::Ascending, sensitivity), asc);
        QCOMPARE(TextTransforms::sortLexicographic(in, SortOrder::Descending, sensitivity), desc);
    }
    void sortLocale()
    {
        const QLocale english(QLocale::English, QLocale::UnitedStates);
        const QString in = QStringLiteral("f\né\ne");  // code-unit order would put é last
        QCOMPARE(TextTransforms::sortLocale(in, SortOrder::Ascending, english),
                 QStringLiteral("e\né\nf"));
        QCOMPARE(TextTransforms::sortLocale(in, SortOrder::Descending, english),
                 QStringLiteral("f\né\ne"));
        QCOMPARE(TextTransforms::sortLocale(QString(), SortOrder::Ascending, english), QString());
    }
    // Non-numeric lines keep their order: first when ascending, last when descending.
    void sortIntegers_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<QString>("asc");
        QTest::addColumn<QString>("desc");
        QTest::newRow("empty") << "" << "" << "";
        QTest::newRow("mixed") << "10\n9\nx\n-3\n\n2 apples" << "x\n\n-3\n2 apples\n9\n10"
                               << "10\n9\n2 apples\n-3\nx\n";
        QTest::newRow("huge") << "99999999999999999999\n1" << "1\n99999999999999999999"
                              << "99999999999999999999\n1";
        QTest::newRow("zeros and signs") << "007\n+5\n-0\n0\n--5" << "--5\n-0\n0\n+5\n007"
                                         << "007\n+5\n-0\n0\n--5";
        QTest::newRow("indent and crlf") << "  20\r\n\t3\r\n" << "\t3\r\n  20\r\n" << "  20\r\n\t3\r\n";
    }
    void sortIntegers()
    {
        QFETCH(QString, in);
        QFETCH(QString, asc);
        QFETCH(QString, desc);
        QCOMPARE(TextTransforms::sortIntegers(in, SortOrder::Ascending), asc);
        QCOMPARE(TextTransforms::sortIntegers(in, SortOrder::Descending), desc);
    }
    void sortDecimals()
    {
        const QString dot = QStringLiteral("1.5\n1.25\n-0.5\nabc\n.75");
        QCOMPARE(TextTransforms::sortDecimals(dot, SortOrder::Ascending, QLatin1Char('.')),
                 QStringLiteral("abc\n-0.5\n.75\n1.25\n1.5"));
        QCOMPARE(TextTransforms::sortDecimals(dot, SortOrder::Descending, QLatin1Char('.')),
                 QStringLiteral("1.5\n1.25\n.75\n-0.5\nabc"));
        const QString comma = QStringLiteral("1,5\n1,25\n2\nn/a\n");
        QCOMPARE(TextTransforms::sortDecimals(comma, SortOrder::Ascending, QLatin1Char(',')),
                 QStringLiteral("n/a\n1,25\n1,5\n2\n"));
        // With a comma separator "1.5" reads as 1, so it ties with "1" and stays first
        QCOMPARE(TextTransforms::sortDecimals(QStringLiteral("1.5\n1\n0,5"), SortOrder::Ascending,
                                              QLatin1Char(',')),
                 QStringLiteral("0,5\n1.5\n1"));
        QCOMPARE(TextTransforms::sortDecimals(QString(), SortOrder::Ascending, QLatin1Char('.')),
                 QString());
    }
    void sortByLength()
    {
        const QString in = QStringLiteral("ccc\na\nbb\nd");
        QCOMPARE(TextTransforms::sortByLength(in, SortOrder::Ascending), QStringLiteral("a\nd\nbb\nccc"));
        QCOMPARE(TextTransforms::sortByLength(in, SortOrder::Descending), QStringLiteral("ccc\nbb\na\nd"));
        // Length counts code points, not UTF-16 units
        const QString wide = QString::fromUcs4(U"ab\n\U00010400", 4);
        QCOMPARE(TextTransforms::sortByLength(wide, SortOrder::Ascending),
                 QString::fromUcs4(U"\U00010400\nab", 4));
        QCOMPARE(TextTransforms::sortByLength(kArabic + "\nab", SortOrder::Ascending),
                 QStringLiteral("ab\n") + kArabic);
    }
    void reverseAndShuffle()
    {
        QCOMPARE(reverseLines(QString()), QString());
        QCOMPARE(reverseLines(QStringLiteral("a\nb\nc\n")), QStringLiteral("c\nb\na\n"));
        QCOMPARE(reverseLines(QStringLiteral("a\r\nb")), QStringLiteral("b\r\na"));
        const QString in = QStringLiteral("1\n2\n3\n4\n5\n6\n7\n8\n");
        const QString out = shuffleLines(in, 42);
        QVERIFY(out.endsWith(QLatin1Char('\n')));
        QStringList sorted = splitLines(out).lines;
        sorted.sort();
        QCOMPARE(sorted, splitLines(in).lines);
        QCOMPARE(shuffleLines(in, 42), out);
    }
    void tabsToSpaces_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<QString>("out");
        QTest::newRow("empty") << "" << "";
        QTest::newRow("tab stops") << "\ta\tb" << "    a   b";
        QTest::newRow("trailing") << "ab\t\n" << "ab  \n";
        QTest::newRow("arabic") << kArabic + "\tx" << kArabic + "   x";
        QTest::newRow("crlf") << "\ta\r\n\tb" << "    a\r\n    b";
    }
    void tabsToSpaces()
    {
        QFETCH(QString, in);
        QFETCH(QString, out);
        QCOMPARE(TextTransforms::tabsToSpaces(in, 4), out);
    }
    void spacesToTabs_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<QString>("all");
        QTest::addColumn<QString>("leading");
        QTest::newRow("empty") << "" << "" << "";
        QTest::newRow("indent") << "        a" << "\t\ta" << "\t\ta";
        QTest::newRow("inner run") << "a   b" << "a\tb" << "a   b";
        QTest::newRow("single space") << "a b\n a" << "a b\n a" << "a b\n a";
        QTest::newRow("partial") << "     a" << "\t a" << "\t a";
        QTest::newRow("leading only") << "    a    b" << "\ta\t b" << "\ta    b";
        QTest::newRow("spaces before tab") << "  \t  a" << "\t  a" << "\t  a";
        QTest::newRow("crlf") << "    a\r\n    b" << "\ta\r\n\tb" << "\ta\r\n\tb";
    }
    void spacesToTabs()
    {
        QFETCH(QString, in);
        QFETCH(QString, all);
        QFETCH(QString, leading);
        QCOMPARE(TextTransforms::spacesToTabs(in, 4, false), all);
        QCOMPARE(TextTransforms::spacesToTabs(in, 4, true), leading);
    }
    void editorInsertBlankLine()
    {
        Document doc;
        Editor editor(&doc);
        editor.setPlainText(QStringLiteral("a\nb\nc"));
        editor.goToLine(2);
        editor.insertBlankLine(true);
        QCOMPARE(editor.toPlainText(), QStringLiteral("a\n\nb\nc"));
        QCOMPARE(editor.currentLine(), 2);
        editor.goToLine(3);
        editor.insertBlankLine(false);
        QCOMPARE(editor.toPlainText(), QStringLiteral("a\n\nb\n\nc"));
        QCOMPARE(editor.currentLine(), 4);
        QVERIFY(doc.isModified());
        editor.undo();
        QCOMPARE(editor.toPlainText(), QStringLiteral("a\n\nb\nc"));
    }

    // ---- group 4: comment and indent -------------------------------------
    void commentLines_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<QString>("commented");
        QTest::newRow("empty single line") << "" << "// ";
        QTest::newRow("indent kept") << "  a\n\tb" << "  // a\n\t// b";
        QTest::newRow("blank lines skipped") << "a\n\n  \nb\n" << "// a\n\n  \n// b\n";
        QTest::newRow("crlf") << "a\r\nb" << "// a\r\n// b";
        QTest::newRow("arabic") << kArabic << "// " + kArabic;
    }
    void commentLines()
    {
        QFETCH(QString, in);
        QFETCH(QString, commented);
        const CommentSyntax cpp{QStringLiteral("//"), QStringLiteral("/*"), QStringLiteral("*/")};
        QCOMPARE(TextTransforms::commentLines(in, cpp), commented);
        QCOMPARE(TextTransforms::uncommentLines(commented, cpp), in);
    }
    void uncommentLines()
    {
        const CommentSyntax py{QStringLiteral("#"), QString(), QString()};
        QCOMPARE(TextTransforms::uncommentLines(QStringLiteral("  #a\n# b\nc # d\n#  e"), py),
                 QStringLiteral("  a\nb\nc # d\n e"));
        QCOMPARE(TextTransforms::uncommentLines(QString(), py), QString());
    }
    void commentLinesBlockFallback()
    {
        const CommentSyntax html{QString(), QStringLiteral("<!--"), QStringLiteral("-->")};
        const QString commented = TextTransforms::commentLines(QStringLiteral("  <p>\n</p>"), html);
        QCOMPARE(commented, QStringLiteral("  <!-- <p> -->\n<!-- </p> -->"));
        QCOMPARE(TextTransforms::uncommentLines(commented, html), QStringLiteral("  <p>\n</p>"));
        const CommentSyntax none;
        QCOMPARE(TextTransforms::commentLines(QStringLiteral("a"), none), QStringLiteral("a"));
    }
    void blockComment_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<QString>("commented");
        QTest::newRow("words") << "a b" << "/* a b */";
        QTest::newRow("whitespace outside") << "  a\nb\n" << "  /* a\nb */\n";
        QTest::newRow("crlf inside") << "a\r\nb" << "/* a\r\nb */";
        QTest::newRow("arabic") << kArabic << "/* " + kArabic + " */";
    }
    void blockComment()
    {
        QFETCH(QString, in);
        QFETCH(QString, commented);
        const CommentSyntax cpp{QStringLiteral("//"), QStringLiteral("/*"), QStringLiteral("*/")};
        QCOMPARE(TextTransforms::blockComment(in, cpp), commented);
        QCOMPARE(blockUncomment(commented, cpp), std::optional<QString>(in));
    }
    void blockUncommentEdges()
    {
        const CommentSyntax cpp{QStringLiteral("//"), QStringLiteral("/*"), QStringLiteral("*/")};
        QCOMPARE(blockUncomment(QStringLiteral("/*x*/"), cpp), std::optional<QString>(QStringLiteral("x")));
        QCOMPARE(blockUncomment(QStringLiteral("a b"), cpp), std::nullopt);
        QCOMPARE(blockUncomment(QStringLiteral("/*/"), cpp), std::nullopt);
        QCOMPARE(blockUncomment(QString(), cpp), std::nullopt);
        QCOMPARE(TextTransforms::blockComment(QStringLiteral("   "), cpp), QStringLiteral("   "));
        const CommentSyntax lineOnly{QStringLiteral("#"), QString(), QString()};
        QCOMPARE(TextTransforms::blockComment(QStringLiteral("a"), lineOnly), QStringLiteral("a"));
        QCOMPARE(blockUncomment(QStringLiteral("a"), lineOnly), std::nullopt);
    }
    void indentLines_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<QString>("indented");
        QTest::addColumn<QString>("unindented");
        QTest::newRow("empty") << "" << "    " << "";
        QTest::newRow("lines") << "a\n  b\n\tc" << "    a\n      b\n    \tc" << "a\nb\nc";
        QTest::newRow("to previous stop") << "      a" << "          a" << "    a";
        QTest::newRow("empty lines skipped") << "a\n\nb\n" << "    a\n\n    b\n" << "a\n\nb\n";
        QTest::newRow("crlf") << "  a\r\nb" << "      a\r\n    b" << "a\r\nb";
    }
    void indentLines()
    {
        QFETCH(QString, in);
        QFETCH(QString, indented);
        QFETCH(QString, unindented);
        QCOMPARE(TextTransforms::indentLines(in, QStringLiteral("    ")), indented);
        QCOMPARE(TextTransforms::unindentLines(in, 4), unindented);
        QCOMPARE(TextTransforms::indentLines(QStringLiteral("a"), QStringLiteral("\t")), QStringLiteral("\ta"));
    }
    // The language definitions supply the delimiters (resources/syntax/*.json).
    void editorCommentSyntaxFromLanguage()
    {
        Document doc;
        Editor editor(&doc);
        editor.setLanguage(QStringLiteral("C++"));
        QCOMPARE(editor.commentSyntax().line, QStringLiteral("//"));
        QCOMPARE(editor.commentSyntax().blockStart, QStringLiteral("/*"));
        QCOMPARE(editor.commentSyntax().blockEnd, QStringLiteral("*/"));
        editor.setLanguage(QStringLiteral("Python"));
        QCOMPARE(editor.commentSyntax().line, QStringLiteral("#"));
    }

    // ---- group 5: insert and clipboard ------------------------------------
    // Notepad++ order: time (no seconds), then the short or long date.
    void formatDateTime()
    {
        const QDateTime when(QDate(2026, 11, 28), QTime(14, 5, 9));
        const QLocale us(QLocale::English, QLocale::UnitedStates);
        const QLocale de(QLocale::German, QLocale::Germany);
        QCOMPARE(TextTransforms::formatDateTime(when, us, false), QStringLiteral("2:05 PM 11/28/26"));
        QCOMPARE(TextTransforms::formatDateTime(when, us, true),
                 QStringLiteral("2:05 PM Saturday, November 28, 2026"));
        QCOMPARE(TextTransforms::formatDateTime(when, de, false), QStringLiteral("14:05 28.11.26"));
        QCOMPARE(TextTransforms::formatDateTime(when, de, true),
                 QStringLiteral("14:05 Samstag, 28. November 2026"));
    }
    void copyPathsAndInsertDateTime()
    {
        regression::TempDir tmp;
        QVERIFY(QDir(tmp.path()).mkdir(QStringLiteral("sub")));
        const QString path = tmp.filePath(QStringLiteral("sub/note.txt"));
        QVERIFY(regression::writeBytes(path, "x"));
        MainWindow mw;
        mw.show();
        Editor *editor = mw.openFile(path);
        QVERIFY(editor);

        QVERIFY(trigger(mw, QStringLiteral("Current Full File Path to Clipboard")));
        QCOMPARE(QGuiApplication::clipboard()->text(), path);
        QVERIFY(trigger(mw, QStringLiteral("Current Filename to Clipboard")));
        QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("note.txt"));
        QVERIFY(trigger(mw, QStringLiteral("Current Dir. Path to Clipboard")));
        QCOMPARE(QGuiApplication::clipboard()->text(), tmp.filePath(QStringLiteral("sub")));

        editor->moveCursor(QTextCursor::End);
        QVERIFY(trigger(mw, QStringLiteral("Date Time (long)")));
        const QString expectedDate = QLocale::system().toString(QDate::currentDate(), QLocale::LongFormat);
        QVERIFY2(editor->toPlainText().startsWith(QLatin1Char('x')), qPrintable(editor->toPlainText()));
        QVERIFY2(editor->toPlainText().endsWith(expectedDate), qPrintable(editor->toPlainText()));
        QVERIFY(editor->document()->isModified());
        editor->undo();
        QCOMPARE(editor->toPlainText(), QStringLiteral("x"));
    }

    // ---- group 6: tools ----------------------------------------------------
    // Reference values from Python's hashlib, base64 and urllib.
    void hashes()
    {
        using CodecTransforms::HashAlgorithm;
        QCOMPARE(hashText(QString(), HashAlgorithm::Md5), QStringLiteral("d41d8cd98f00b204e9800998ecf8427e"));
        QCOMPARE(hashText(QStringLiteral("abc"), HashAlgorithm::Sha1),
                 QStringLiteral("a9993e364706816aba3e25717850c26c9cd0d89d"));
        QCOMPARE(hashText(QStringLiteral("abc"), HashAlgorithm::Sha256),
                 QStringLiteral("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
        QCOMPARE(hashText(QStringLiteral("abc"), HashAlgorithm::Sha512),
                 QStringLiteral("ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
                                "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f"));
        QCOMPARE(hashText(kArabic, HashAlgorithm::Md5), QStringLiteral("9530db02d3c8217be00c1aa98ec51861"));
    }
    void hashFileReadsBytes()
    {
        using CodecTransforms::HashAlgorithm;
        regression::TempDir tmp;
        const QString path = tmp.filePath(QStringLiteral("crlf.txt"));
        QVERIFY(regression::writeBytes(path, "abc\r\n"));
        QCOMPARE(CodecTransforms::hashFile(path, HashAlgorithm::Sha256),
                 std::optional<QString>(
                     QStringLiteral("552bab6864c7a7b69a502ed1854b9245c0e1a30f008aaa0b281da62585fdb025")));
        QCOMPARE(regression::readBytes(path), QByteArray("abc\r\n"));  // read-only
        QCOMPARE(CodecTransforms::hashFile(tmp.filePath(QStringLiteral("missing")), HashAlgorithm::Md5),
                 std::nullopt);
    }
    void base64()
    {
        QCOMPARE(base64Encode(QStringLiteral("Hello")), QStringLiteral("SGVsbG8="));
        QCOMPARE(base64Encode(kArabic), QStringLiteral("2YXYsdit2KjYpw=="));
        QCOMPARE(base64Encode(QString()), QString());
        QCOMPARE(base64Decode(QStringLiteral("2YXYsdit2KjYpw==")), std::optional<QString>(kArabic));
        QCOMPARE(base64Decode(QStringLiteral("SGVs\r\nbG8=")), std::optional<QString>(QStringLiteral("Hello")));
        QCOMPARE(base64Decode(QString()), std::optional<QString>(QString()));
        QCOMPARE(base64Decode(QStringLiteral("SGV$bG8=")), std::nullopt);   // bad character
        QCOMPARE(base64Decode(QStringLiteral("/w==")), std::nullopt);       // 0xFF is not UTF-8
    }
    void urlCoding()
    {
        const QString plain = QStringLiteral("a b&c/é~-._");
        QCOMPARE(urlEncode(plain), QStringLiteral("a%20b%26c%2F%C3%A9~-._"));
        QCOMPARE(urlDecode(QStringLiteral("a%20b%26c%2F%C3%A9~-._")), std::optional<QString>(plain));
        QCOMPARE(urlDecode(QStringLiteral("a+b")), std::optional<QString>(QStringLiteral("a+b")));
        QCOMPARE(urlDecode(QStringLiteral("%zz")), std::nullopt);
        QCOMPARE(urlDecode(QStringLiteral("abc%E")), std::nullopt);
        QCOMPARE(urlDecode(QStringLiteral("%FF")), std::nullopt);
        QCOMPARE(urlEncode(kArabic), QStringLiteral("%D9%85%D8%B1%D8%AD%D8%A8%D8%A7"));
    }
    void hexCoding()
    {
        QCOMPARE(asciiToHex(QStringLiteral("Hi\n")), QStringLiteral("48690A"));
        QCOMPARE(asciiToHex(kArabic), QStringLiteral("D985D8B1D8ADD8A8D8A7"));
        QCOMPARE(hexToAscii(QStringLiteral("48 69\r\n0a")), std::optional<QString>(QStringLiteral("Hi\n")));
        QCOMPARE(hexToAscii(QStringLiteral("D985D8B1D8ADD8A8D8A7")), std::optional<QString>(kArabic));
        QCOMPARE(hexToAscii(QString()), std::optional<QString>(QString()));
        QCOMPARE(hexToAscii(QStringLiteral("486")), std::nullopt);   // odd digit count
        QCOMPARE(hexToAscii(QStringLiteral("4G")), std::nullopt);
        QCOMPARE(hexToAscii(QStringLiteral("FF")), std::nullopt);    // not UTF-8
    }
    // With nothing selected the digest covers the whole document; the dialog
    // shows it and leaves the document untouched.
    void hashDialogShowsDocumentDigest()
    {
        MainWindow mw;
        mw.show();
        Editor *editor = mw.currentEditor();
        QVERIFY(editor);
        editor->setPlainText(QStringLiteral("abc"));
        QString shown;
        QTimer::singleShot(0, &mw, [&shown]() {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog) return;
            if (auto *view = dialog->findChild<QPlainTextEdit *>()) shown = view->toPlainText();
            dialog->reject();
        });
        auto *action = mw.findChild<QAction *>(QStringLiteral("hashText SHA-1"));
        QVERIFY(action);
        action->trigger();
        QCOMPARE(shown, QStringLiteral("a9993e364706816aba3e25717850c26c9cd0d89d"));
        QCOMPARE(editor->toPlainText(), QStringLiteral("abc"));
    }
    // Invalid input leaves the selection exactly as it was and says so.
    void invalidDecodeLeavesTextUnchanged()
    {
        MainWindow mw;
        mw.show();
        Editor *editor = mw.currentEditor();
        QVERIFY(editor);
        editor->setPlainText(QStringLiteral("not base64!"));
        editor->selectAll();
        textDoc(*editor)->setModified(false);
        editor->document()->setModified(false);
        QVERIFY(trigger(mw, QStringLiteral("Base64 Decode")));
        QCOMPARE(editor->toPlainText(), QStringLiteral("not base64!"));
        QVERIFY(!editor->document()->isModified());
        QVERIFY(trigger(mw, QStringLiteral("Base64 Encode")));
        QCOMPARE(editor->toPlainText(), QStringLiteral("bm90IGJhc2U2NCE="));
        QVERIFY(trigger(mw, QStringLiteral("Base64 Decode")));
        QCOMPARE(editor->toPlainText(), QStringLiteral("not base64!"));
    }

    // ---- group 7: search and navigation -----------------------------------
    void matchingBrace()
    {
        const QString text = QStringLiteral("a(b[c]d)e");
        QCOMPARE(TextTransforms::matchingBrace(text, 1).match, 7);
        QCOMPARE(TextTransforms::matchingBrace(text, 8).brace, 7);   // just after ')'
        QCOMPARE(TextTransforms::matchingBrace(text, 8).match, 1);
        QCOMPARE(TextTransforms::matchingBrace(text, 4).match, 5);   // 'c' is no brace; '[' before it is
        QVERIFY(!TextTransforms::matchingBrace(text, 0).isValid());
        QVERIFY(!TextTransforms::matchingBrace(QStringLiteral("((a)"), 0).isValid());
        QVERIFY(!TextTransforms::matchingBrace(QString(), 0).isValid());
        QCOMPARE(TextTransforms::matchingBrace(QStringLiteral("{[}]"), 0).match, 2);
    }
    void bookmarkedLineFunctions()
    {
        QCOMPARE(keepLines(QStringLiteral("a\nb\nc\n"), {0, 2}, true), QStringLiteral("a\nc\n"));
        QCOMPARE(keepLines(QStringLiteral("a\nb\nc\n"), {0, 2}, false), QStringLiteral("b\n"));
        QCOMPARE(keepLines(QStringLiteral("a\nb"), {0, 1}, false), QString());
        QCOMPARE(keepLines(QStringLiteral("a\r\nb\r\nc"), {1, 9}, true), QStringLiteral("b"));
        QCOMPARE(copyLines(QStringLiteral("a\r\nb\r\nc"), {0, 2}), QStringLiteral("a\r\nc\r\n"));
        QCOMPARE(copyLines(QStringLiteral("a\nb"), {}), QString());
        QCOMPARE(replaceLines(QStringLiteral("a\nb\nc"), {1}, QStringLiteral("X\r\nY\r\n")),
                 QStringLiteral("a\nX\nY\nc"));
        QCOMPARE(replaceLines(QStringLiteral("a\nb\nc"), {0, 2}, QStringLiteral("Z")), QStringLiteral("Z\nb\nZ"));
        QCOMPARE(replaceLines(kArabic, {0}, QString()), QString());
    }
    void selectAndFindNextUsesWordUnderCursor()
    {
        MainWindow mw;
        mw.show();
        Editor *editor = mw.currentEditor();
        editor->setPlainText(QStringLiteral("foo bar Foo foo"));
        QTextCursor cursor = editor->textCursor();
        cursor.setPosition(1);
        editor->setTextCursor(cursor);
        QVERIFY(trigger(mw, QStringLiteral("Select and Find Next")));
        QCOMPARE(editor->textCursor().selectionStart(), 12);  // case-sensitive: skips "Foo"
        QCOMPARE(editor->textCursor().selectedText(), QStringLiteral("foo"));
        QVERIFY(trigger(mw, QStringLiteral("Select and Find Previous")));
        QCOMPARE(editor->textCursor().selectionStart(), 0);
        // Backward wrap: from the first match, Previous goes round to the last.
        // Editor::findNext used to restart a backward search at position 0.
        QVERIFY(trigger(mw, QStringLiteral("Select and Find Previous")));
        QCOMPARE(editor->textCursor().selectionStart(), 12);
    }
    void selectAllBetweenMatchingBraces()
    {
        MainWindow mw;
        mw.show();
        Editor *editor = mw.currentEditor();
        editor->setPlainText(QStringLiteral("x(a(b)c)y"));
        QTextCursor cursor = editor->textCursor();
        cursor.setPosition(1);
        editor->setTextCursor(cursor);
        QVERIFY(trigger(mw, QStringLiteral("Select All between Matching Braces")));
        QCOMPARE(editor->textCursor().selectedText(), QStringLiteral("(a(b)c)"));
    }
    void bookmarkLineOperations()
    {
        MainWindow mw;
        mw.show();
        Editor *editor = mw.currentEditor();
        editor->setPlainText(QStringLiteral("l1\nl2\nl3\nl4"));
        editor->setBookmarks({2, 4});
        QVERIFY(trigger(mw, QStringLiteral("Copy Bookmarked Lines")));
        QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("l2\nl4\n"));
        QVERIFY(trigger(mw, QStringLiteral("Inverse Bookmark")));
        QCOMPARE(editor->bookmarks(), (QSet<int>{1, 3}));
        QVERIFY(trigger(mw, QStringLiteral("Remove Unmarked Lines")));
        QCOMPARE(editor->toPlainText(), QStringLiteral("l1\nl3"));
        QCOMPARE(editor->bookmarks(), (QSet<int>{1, 2}));
        editor->undo();
        QCOMPARE(editor->toPlainText(), QStringLiteral("l1\nl2\nl3\nl4"));

        editor->setBookmarks({1, 3});
        QGuiApplication::clipboard()->setText(QStringLiteral("A\nB\n"));
        QVERIFY(trigger(mw, QStringLiteral("Paste to (Replace) Bookmarked Lines")));
        QCOMPARE(editor->toPlainText(), QStringLiteral("A\nB\nl2\nA\nB\nl4"));
        QCOMPARE(editor->bookmarks(), (QSet<int>{1, 4}));
        QVERIFY(trigger(mw, QStringLiteral("Cut Bookmarked Lines")));
        QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("A\nA\n"));
        QCOMPARE(editor->toPlainText(), QStringLiteral("B\nl2\nB\nl4"));
        QVERIFY(editor->bookmarks().isEmpty());
        editor->setBookmarks({4});
        QVERIFY(trigger(mw, QStringLiteral("Remove Bookmarked Lines")));
        QCOMPARE(editor->toPlainText(), QStringLiteral("B\nl2\nB"));
    }
    void replaceAndCountInSelection()
    {
        MainWindow mw;
        mw.show();
        Editor *editor = mw.currentEditor();
        editor->setPlainText(QStringLiteral("a a\na a"));
        QTextCursor cursor = editor->textCursor();
        cursor.setPosition(4);
        cursor.setPosition(7, QTextCursor::KeepAnchor);
        editor->setTextCursor(cursor);
        SearchDialog *dialog = searchDialog(mw);
        QVERIFY(dialog);
        dialog->setSearchText(QStringLiteral("a"));
        dialog->setReplaceText(QStringLiteral("b"));
        dialog->setInSelection(true);
        emit dialog->replaceAll();
        QCOMPARE(editor->toPlainText(), QStringLiteral("a a\nb b"));
        editor->undo();
        QCOMPARE(editor->toPlainText(), QStringLiteral("a a\na a"));
        editor->selectAll();
        dialog->setInSelection(false);
        emit dialog->replaceAll();
        QCOMPARE(editor->toPlainText(), QStringLiteral("b b\nb b"));
    }
    void markInSelection()
    {
        MainWindow mw;
        mw.show();
        Editor *editor = mw.currentEditor();
        editor->setPlainText(QStringLiteral("a a\na a"));
        QTextCursor cursor = editor->textCursor();
        cursor.setPosition(0);
        cursor.setPosition(3, QTextCursor::KeepAnchor);
        editor->setTextCursor(cursor);
        SearchDialog *dialog = searchDialog(mw);
        dialog->showTab(3);
        dialog->setSearchText(QStringLiteral("a"));
        dialog->setInSelection(true);
        emit dialog->markAll();
        QCOMPARE(editor->markCount(), 2);
        dialog->setInSelection(false);
        emit dialog->markAll();
        QCOMPARE(editor->markCount(), 4);
    }
    // In memory only: a file-backed document is changed in its editor and
    // marked modified, and its file on disk is left alone.
    void replaceAllInAllOpenedDocuments()
    {
        regression::TempDir tmp;
        const QString path = tmp.filePath(QStringLiteral("disk.txt"));
        QVERIFY(regression::writeBytes(path, "x on disk\n"));
        MainWindow mw;
        mw.show();
        Editor *first = mw.currentEditor();
        first->setPlainText(QStringLiteral("x1 x"));
        Editor *second = mw.openFile(path);
        QVERIFY(second && second != first);
        SearchDialog *dialog = searchDialog(mw);
        dialog->setSearchText(QStringLiteral("x"));
        dialog->setReplaceText(QStringLiteral("y"));
        emit dialog->replaceAllInAllOpen();
        QCOMPARE(first->toPlainText(), QStringLiteral("y1 y"));
        QCOMPARE(second->toPlainText(), QStringLiteral("y on disk\n"));
        QVERIFY(second->document()->isModified());
        QCOMPARE(regression::readBytes(path), QByteArray("x on disk\n"));
    }

    // ---- group 8: character sets -----------------------------------------
    void characterSetsAreAvailableAndGrouped()
    {
        const QList<CharacterSets::Group> groups = CharacterSets::available();
        QStringList regions;
        QStringList encodings;
        for (const CharacterSets::Group &group : groups) {
            regions << group.region;
            QVERIFY2(!group.sets.isEmpty(), qPrintable(group.region));
            for (const CharacterSets::Set &set : group.sets) {
                QVERIFY2(!TextCodec::canonicalName(set.encoding).isEmpty(), qPrintable(set.encoding));
                encodings << TextCodec::canonicalName(set.encoding);
            }
        }
        QCOMPARE(regions, (QStringList{"Arabic", "Baltic", "Celtic", "Cyrillic", "Central European",
                                       "Chinese", "Eastern European", "Greek", "Hebrew", "Japanese",
                                       "Korean", "North European", "Thai", "Turkish",
                                       "Western European", "Vietnamese"}));
        for (const char *name : {"windows-1256", "ISO-8859-6", "KOI8-R", "Big5", "Shift_JIS",
                                 "windows-1252", "ISO-8859-15", "TIS-620", "windows-1258"}) {
            QVERIFY2(encodings.contains(TextCodec::canonicalName(QString::fromLatin1(name))), name);
        }
        // A code page this ICU lacks is left out rather than offered and failing
        const bool oem720 = !TextCodec::canonicalName(QStringLiteral("ibm-720")).isEmpty();
        QCOMPARE(encodings.contains(TextCodec::canonicalName(QStringLiteral("ibm-720"))), oem720);
    }
    // Reopening in a code page re-decodes the file's bytes: the text changes,
    // the document is clean, and the file on disk is untouched.
    void characterSetReopensTheFile()
    {
        regression::TempDir tmp;
        const QString path = tmp.filePath(QStringLiteral("arabic.txt"));
        const QByteArray cp1256("\xE3\xD1\xCD\xC8\xC7\n");  // marhaban in windows-1256
        QVERIFY(regression::writeBytes(path, cp1256));
        MainWindow mw;
        mw.show();
        Editor *editor = mw.openFile(path);
        QVERIFY(editor);
        QVERIFY(trigger(mw, QStringLiteral("Windows-1256")));
        QCOMPARE(editor->document()->encoding(), TextCodec::canonicalName(QStringLiteral("windows-1256")));
        QCOMPARE(editor->toPlainText(), kArabic + QLatin1Char('\n'));
        QVERIFY(!editor->document()->isModified());
        QCOMPARE(regression::readBytes(path), cp1256);
    }

    // ---- group 9: command line ----------------------------------------------
    void normalizeNppArguments()
    {
        QCOMPARE(CommandLine::normalizeNppArguments({"pe", "-n12", "-c5", "-p100", "-lpython", "-ro", "f.txt"}),
                 (QStringList{"pe", "--line=12", "--column=5", "--position=100", "--language=python",
                              "--read-only", "f.txt"}));
        // Left alone: no value, a value of the wrong kind, the program name, after --
        QCOMPARE(CommandLine::normalizeNppArguments({"-n12", "-n", "-nx", "-l", "-rox", "--", "-n3"}),
                 (QStringList{"-n12", "-n", "-nx", "-l", "-rox", "--", "-n3"}));
    }
    void parseLaunchRequest_data()
    {
        QTest::addColumn<QStringList>("args");
        QTest::addColumn<int>("line");
        QTest::addColumn<int>("column");
        QTest::addColumn<int>("position");
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("readOnly");
        QTest::newRow("npp syntax") << QStringList{"-n12", "-c5", "-lcpp", "-ro", "a.txt"}
                                    << 12 << 5 << -1 << "cpp" << true;
        QTest::newRow("long syntax") << QStringList{"--line=7", "--column", "2", "--position=9", "a.txt"}
                                     << 7 << 2 << 9 << "" << false;
        QTest::newRow("existing goto") << QStringList{"--goto", "4", "a.txt"} << 4 << 0 << -1 << "" << false;
        QTest::newRow("short with space") << QStringList{"-n", "3", "a.txt"} << 3 << 0 << -1 << "" << false;
        QTest::newRow("invalid values") << QStringList{"--line=0", "--column=-3", "--position=x", "a.txt"}
                                        << 0 << 0 << -1 << "" << false;
    }
    void parseLaunchRequest()
    {
        QFETCH(QStringList, args);
        QCommandLineParser parser;
        CommandLine::addOptions(parser);
        QVERIFY2(parser.parse(CommandLine::normalizeNppArguments(QStringList{"pe"} + args)),
                 qPrintable(parser.errorText()));
        const LaunchRequest request = CommandLine::request(parser);
        QCOMPARE(request.files, QStringList{QFileInfo(QStringLiteral("a.txt")).absoluteFilePath()});
        QTEST(request.options.line, "line");
        QTEST(request.options.column, "column");
        QTEST(request.options.position, "position");
        QTEST(request.options.language, "language");
        QTEST(request.options.readOnly, "readOnly");
    }
    void resolveLanguageName()
    {
        const LanguageManager &languages = LanguageManager::instance();
        QCOMPARE(languages.resolveLanguageName(QStringLiteral("python")), QStringLiteral("Python"));
        QCOMPARE(languages.resolveLanguageName(QStringLiteral("cpp")), QStringLiteral("C++"));
        QCOMPARE(languages.resolveLanguageName(QStringLiteral("JavaScript")), QStringLiteral("JavaScript"));
        QCOMPARE(languages.resolveLanguageName(QStringLiteral("normal")), QStringLiteral("Plain Text"));
        QCOMPARE(languages.resolveLanguageName(QStringLiteral("bogus")), QString());
        QCOMPARE(languages.resolveLanguageName(QString()), QString());
    }
    void applyOpenOptions()
    {
        regression::TempDir tmp;
        const QString path = tmp.filePath(QStringLiteral("opts.txt"));
        QVERIFY(regression::writeBytes(path, "first\nsecond line\nthird\n"));
        MainWindow mw;
        mw.show();
        Editor *editor = mw.openFile(path);
        QVERIFY(editor);
        OpenOptions options;
        options.line = 2;
        options.column = 4;
        options.language = QStringLiteral("cpp");
        options.readOnly = true;
        mw.applyOpenOptions(editor, options);
        QCOMPARE(editor->currentLine(), 2);
        QCOMPARE(editor->currentColumn(), 4);
        QCOMPARE(editor->language(), QStringLiteral("C++"));
        QVERIFY(editor->document()->isReadOnly());
        QVERIFY(editor->isReadOnly());

        OpenOptions byPosition;
        byPosition.position = 8;  // 0-based: "first\nse|cond"
        byPosition.column = 99;   // clamped to the line
        mw.applyOpenOptions(editor, byPosition);
        QCOMPARE(editor->textCursor().position(), 8);
        OpenOptions far;
        far.line = 99;
        far.column = 99;
        mw.applyOpenOptions(editor, far);
        QCOMPARE(editor->currentLine(), 2);  // a line past the end is ignored
        QCOMPARE(regression::readBytes(path), QByteArray("first\nsecond line\nthird\n"));
    }
    // The extra options travel in the forwarding request; an old holder that
    // does not know them still gets the files and the line.
    void forwardCarriesOpenOptions()
    {
        // A short scratch XDG_RUNTIME_DIR (0700): the ctest one can exceed the
        // 104-byte socket path limit, and the real one must never be reached.
        QTemporaryDir runtime(QStringLiteral("/tmp/pe-rt-XXXXXX"));
        QVERIFY(runtime.isValid());
        QVERIFY(QFile::setPermissions(runtime.path(), QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        const QByteArray previousRuntime = qgetenv("XDG_RUNTIME_DIR");
        qputenv("XDG_RUNTIME_DIR", runtime.path().toUtf8());
        const auto restore = qScopeGuard([&] { qputenv("XDG_RUNTIME_DIR", previousRuntime); });
        QVERIFY2(SingleInstance::serverName().startsWith(runtime.path() + "/"),
                 qPrintable(SingleInstance::serverName()));

        SingleInstance holder;
        QVERIFY(holder.acquire());
        QVERIFY(holder.listen());
        QStringList received;
        OpenOptions got;
        QObject::connect(&holder, &SingleInstance::openRequested, &holder,
                         [&](const QStringList &files, const OpenOptions &options) {
                             received = files;
                             got = options;
                         }, Qt::QueuedConnection);
        OpenOptions sent;
        sent.line = 5;
        sent.column = 3;
        sent.position = 0;
        sent.language = QStringLiteral("python");
        sent.readOnly = true;
        std::atomic<bool> done{false};
        bool acked = false;
        std::thread client([&] {
            acked = SingleInstance::forward({QStringLiteral("/tmp/forwarded.txt")}, sent);
            done = true;
        });
        QTRY_VERIFY_WITH_TIMEOUT(done.load(), 10000);
        client.join();
        QVERIFY(acked);
        QTRY_COMPARE_WITH_TIMEOUT(received, QStringList{QStringLiteral("/tmp/forwarded.txt")}, 3000);
        QCOMPARE(got.line, 5);
        QCOMPARE(got.column, 3);
        QCOMPARE(got.position, 0);
        QCOMPARE(got.language, QStringLiteral("python"));
        QVERIFY(got.readOnly);
    }

    // Qt's text controls claim some Ctrl letters on X11 (Ctrl+K deletes to the
    // end of the line), which would starve a menu shortcut. Typed into a
    // focused editor, each Notepad++ shortcut must reach its action.
    void shortcutsReachActionsFromTheEditor()
    {
        MainWindow mw;
        mw.show();
        QVERIFY(QTest::qWaitForWindowActive(&mw));
        Editor *editor = mw.currentEditor();
        QVERIFY(editor);
        editor->setFocus();
        editor->setLanguage(QStringLiteral("C++"));
        editor->setPlainText(QStringLiteral("abc def"));

        QTest::keyClick(editor, Qt::Key_K, Qt::ControlModifier);
        QCOMPARE(editor->toPlainText(), QStringLiteral("// abc def"));
        QTest::keyClick(editor, Qt::Key_Q, Qt::ControlModifier | Qt::ShiftModifier);
        QCOMPARE(editor->toPlainText(), QStringLiteral("/* // abc def */"));
        editor->selectAll();
        QTest::keyClick(editor, Qt::Key_U, Qt::AltModifier);
        QCOMPARE(editor->toPlainText(), QStringLiteral("/* // Abc Def */"));
        QTest::keyClick(editor, Qt::Key_I, Qt::ControlModifier | Qt::AltModifier);
        QCOMPARE(editor->toPlainText(), QStringLiteral("/* // aBC dEF */"));
        QTest::keyClick(editor, Qt::Key_Return, Qt::ControlModifier | Qt::AltModifier);
        QCOMPARE(editor->toPlainText(), QStringLiteral("\n/* // aBC dEF */"));
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

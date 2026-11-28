#ifndef TEXTTRANSFORMS_H
#define TEXTTRANSFORMS_H

#include <QDateTime>
#include <QLocale>
#include <QString>
#include <QStringList>

#include <optional>

// Pure text transforms behind the Notepad++-parity Edit commands. QString in,
// QString out, no widgets: the editor core is moving from QPlainTextEdit to
// Scintilla, and only the adapter that applies a result (Editor::replaceRange)
// has to change with it.
//
// Line functions split on any EOL (CRLF, CR or LF) and rejoin with the first
// EOL found in the input, so mixed endings come out as that one. A trailing EOL
// is not a line of its own: it is kept as it was and never sorted or removed.
namespace TextTransforms {

// The text split into lines, with the EOL to rejoin them and whether the text
// ended with one.
struct SplitText {
    QStringList lines;
    QString eol = QStringLiteral("\n");
    bool trailingEol = false;
};
SplitText splitLines(const QString &text);
QString joinLines(const SplitText &split);

// Line operations
QString removeEmptyLines(const QString &text);            // length 0 only
QString removeBlankLines(const QString &text);            // empty or whitespace only
QString removeConsecutiveDuplicateLines(const QString &text);

// Blank operations
QString trimTrailing(const QString &text);
QString trimLeading(const QString &text);
QString trimBoth(const QString &text);
QString eolToSpace(const QString &text);                   // keeps a trailing EOL
QString trimBothAndEolToSpace(const QString &text);
// Columns advance to the next multiple of tabWidth at a tab. Runs of two or
// more spaces that reach a tab stop become a tab; a single space is left
// alone. leadingOnly limits the conversion to each line's indentation.
QString tabsToSpaces(const QString &text, int tabWidth);
QString spacesToTabs(const QString &text, int tabWidth, bool leadingOnly);

// Line order. Sorts are stable. Integer and decimal sorts read the number at
// the start of each line (after blanks); lines with no number keep their
// relative order and go first when ascending, last when descending. Integers
// compare by digits, so any length works; decimals take one separator, no
// thousands grouping. Length counts code points.
enum class SortOrder { Ascending, Descending };
QString sortLexicographic(const QString &text, SortOrder order, Qt::CaseSensitivity cs);
QString sortLocale(const QString &text, SortOrder order, const QLocale &locale);
QString sortIntegers(const QString &text, SortOrder order);
QString sortDecimals(const QString &text, SortOrder order, QChar decimalSeparator);
QString sortByLength(const QString &text, SortOrder order);
QString reverseLines(const QString &text);
QString shuffleLines(const QString &text, quint32 seed);

// Indentation. When there is more than one line, empty lines are left alone.
// Unindent removes a leading tab, or spaces back to the previous tab stop.
QString indentLines(const QString &text, const QString &unit);
QString unindentLines(const QString &text, int tabWidth);

// Comment delimiters of a language (resources/syntax/*.json).
struct CommentSyntax {
    QString line;
    QString blockStart;
    QString blockEnd;
    bool hasLine() const { return !line.isEmpty(); }
    bool hasBlock() const { return !blockStart.isEmpty() && !blockEnd.isEmpty(); }
};
// Single-line comment: the token and a space go after each line's indent.
// Blank lines are skipped when there is more than one line. A language with
// no line comment wraps each line in its block delimiters instead.
QString commentLines(const QString &text, const CommentSyntax &syntax);
// Removes one comment token (and one space after it) from each line that
// starts with it after its indent; other lines are unchanged.
QString uncommentLines(const QString &text, const CommentSyntax &syntax);
// Block comment: the delimiters go inside the surrounding whitespace, each
// with a space. Unchanged when the language has no block delimiters or the
// text is only whitespace.
QString blockComment(const QString &text, const CommentSyntax &syntax);
// Removes one pair of block delimiters; nullopt when the text is not wrapped.
std::optional<QString> blockUncomment(const QString &text, const CommentSyntax &syntax);

// Notepad++ Insert Date Time: the time without seconds, a space, then the
// date, both in the locale's format (short date, or long date for longFormat).
QString formatDateTime(const QDateTime &when, const QLocale &locale, bool longFormat);

// Case, per code point, so surrogate pairs are handled and characters with no
// case (Arabic, CJK, digits) pass through unchanged. Proper: a letter that
// follows no letter or digit starts a word (an apostrophe inside a word does
// not). Sentence: a sentence starts at the beginning, after . ! ? and after a
// blank line. Blend keeps existing capitals; the plain form lowercases the rest.
QString properCase(const QString &text, bool blend);
QString sentenceCase(const QString &text, bool blend);
QString invertCase(const QString &text);
QString randomCase(const QString &text, quint32 seed);

} // namespace TextTransforms

#endif // TEXTTRANSFORMS_H

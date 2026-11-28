#ifndef TEXTTRANSFORMS_H
#define TEXTTRANSFORMS_H

#include <QString>
#include <QStringList>

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

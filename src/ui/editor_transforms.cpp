// Editor side of the Notepad++-style transforms. The transforms themselves are
// pure functions (core/texttransforms.h); this file only picks the text they
// read and writes their result back. textInRange and replaceRange are the only
// places that touch QTextDocument, so the Scintilla port rewrites only them.
#include "editor.h"
#include "syntax/highlighter.h"

#include <QTextBlock>
#include <QTextCursor>

QString Editor::textInRange(int start, int end) const
{
    QTextCursor cursor(QPlainTextEdit::document());
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    QString text = cursor.selectedText();
    text.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
    return text;
}

// One edit block, so the change is one undo step. It is a user edit, not a
// synced one: the modificationChanged hook marks the Document modified.
void Editor::replaceRange(int start, int end, const QString &text, bool selectResult)
{
    QTextCursor cursor(QPlainTextEdit::document());
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    cursor.beginEditBlock();
    cursor.insertText(text);
    cursor.endEditBlock();
    if (selectResult) {
        cursor.setPosition(start);
        cursor.setPosition(start + int(text.size()), QTextCursor::KeepAnchor);
    }
    setTextCursor(cursor);
}

QPair<int, int> Editor::scopeRange(TransformScope scope) const
{
    const QTextCursor cursor = textCursor();
    const QTextDocument *doc = QPlainTextEdit::document();
    if (scope == TransformScope::Selection)
        return {cursor.selectionStart(), cursor.selectionEnd()};
    if (!cursor.hasSelection() && scope == TransformScope::LinesOrDocument)
        return {0, doc->characterCount() - 1};

    const QTextBlock first = doc->findBlock(cursor.selectionStart());
    QTextBlock last = doc->findBlock(cursor.selectionEnd());
    // A selection that ends at column 0 of a later line does not include it,
    // as in Notepad++.
    if (cursor.hasSelection() && last != first && cursor.selectionEnd() == last.position())
        last = last.previous();
    return {first.position(), last.position() + last.length() - 1};
}

// Notepad++ Insert Blank Line Above/Below Current: an empty line with no
// indentation, and the cursor on it.
void Editor::insertBlankLine(bool above)
{
    if (isReadOnly())
        return;
    const QTextBlock block = textCursor().block();
    const int at = above ? block.position() : block.position() + block.length() - 1;
    replaceRange(at, at, QStringLiteral("\n"), false);
    QTextCursor cursor = textCursor();
    cursor.setPosition(above ? at : at + 1);
    setTextCursor(cursor);
}

TextTransforms::CommentSyntax Editor::commentSyntax() const
{
    return m_highlighter->commentSyntax();
}

bool Editor::applyTextTransform(TransformScope scope, const TextTransform &fn)
{
    if (isReadOnly() || !fn)
        return false;
    const QPair<int, int> range = scopeRange(scope);
    if (scope == TransformScope::Selection && range.first == range.second)
        return false;
    const QString before = textInRange(range.first, range.second);
    const QString after = fn(before);
    if (after == before)
        return false;
    const bool hadSelection = textCursor().hasSelection();
    const int caret = textCursor().position();
    replaceRange(range.first, range.second, after, hadSelection);
    if (!hadSelection) {
        // Keep the caret where it was rather than at the end of the new text
        QTextCursor cursor = textCursor();
        cursor.setPosition(qMin(caret, QPlainTextEdit::document()->characterCount() - 1));
        setTextCursor(cursor);
    }
    return true;
}

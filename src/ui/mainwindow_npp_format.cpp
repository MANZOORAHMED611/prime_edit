// Notepad++ Comment/Uncomment and Indent commands. The comment delimiters come
// from the language definition (resources/syntax/*.json), through the
// highlighter that loaded it.
#include "mainwindow.h"
#include "editor.h"
#include "core/document.h"
#include "core/texttransforms.h"

#include <QAction>
#include <QMenu>
#include <QStatusBar>

using TextTransforms::CommentSyntax;

// Shortcuts: Single Line Comment Ctrl+K and Block Comment Ctrl+Shift+Q as in
// Notepad++. Single Line Uncomment has none: its Notepad++ key Ctrl+Shift+K
// is PrimeEdit's Delete Line. Indent has none: Tab and Shift+Tab belong to
// the editor, and a menu shortcut would take them from typing.
void MainWindow::setupCommentAndIndentMenus()
{
    QMenu *commentMenu = m_editMenu->addMenu(tr("Comment/Uncomment"));
    commentMenu->addAction(tr("Single Line Comment"), this, &MainWindow::singleLineComment,
                           QKeySequence(Qt::CTRL | Qt::Key_K));
    commentMenu->addAction(tr("Single Line Uncomment"), this, &MainWindow::singleLineUncomment);
    commentMenu->addAction(tr("Block Comment"), this, &MainWindow::blockComment,
                           QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Q));
    commentMenu->addAction(tr("Block Uncomment"), this, &MainWindow::blockUncomment);

    QMenu *indentMenu = m_editMenu->addMenu(tr("&Indent"));
    indentMenu->addAction(tr("Increase Line Indent"), this, &MainWindow::increaseIndent);
    indentMenu->addAction(tr("Decrease Line Indent"), this, &MainWindow::decreaseIndent);
}

Editor *MainWindow::commentableEditor(bool needBlock)
{
    Editor *editor = currentEditor();
    if (!editor)
        return nullptr;
    const CommentSyntax syntax = editor->commentSyntax();
    if (needBlock ? syntax.hasBlock() : (syntax.hasLine() || syntax.hasBlock()))
        return editor;
    const QString language = editor->language().isEmpty() ? tr("Plain Text") : editor->language();
    showNotification(needBlock ? tr("%1 has no block comment syntax.").arg(language)
                               : tr("%1 has no comment syntax.").arg(language));
    return nullptr;
}

void MainWindow::singleLineComment()
{
    if (Editor *editor = commentableEditor(false)) {
        const CommentSyntax syntax = editor->commentSyntax();
        transformCurrent(TransformScope::LinesOrCurrentLine,
                         [syntax](const QString &t) { return TextTransforms::commentLines(t, syntax); });
    }
}

void MainWindow::singleLineUncomment()
{
    if (Editor *editor = commentableEditor(false)) {
        const CommentSyntax syntax = editor->commentSyntax();
        transformCurrent(TransformScope::LinesOrCurrentLine,
                         [syntax](const QString &t) { return TextTransforms::uncommentLines(t, syntax); });
    }
}

// The selection, or the current line when nothing is selected.
void MainWindow::blockComment()
{
    if (Editor *editor = commentableEditor(true)) {
        const CommentSyntax syntax = editor->commentSyntax();
        const TransformScope scope = editor->textCursor().hasSelection()
                                         ? TransformScope::Selection
                                         : TransformScope::LinesOrCurrentLine;
        transformCurrent(scope, [syntax](const QString &t) { return TextTransforms::blockComment(t, syntax); });
    }
}

void MainWindow::blockUncomment()
{
    Editor *editor = commentableEditor(true);
    if (!editor)
        return;
    const CommentSyntax syntax = editor->commentSyntax();
    const TransformScope scope = editor->textCursor().hasSelection() ? TransformScope::Selection
                                                                     : TransformScope::LinesOrCurrentLine;
    bool wrapped = true;
    transformCurrent(scope, [syntax, &wrapped](const QString &t) {
        const std::optional<QString> inner = TextTransforms::blockUncomment(t, syntax);
        wrapped = inner.has_value();
        return inner.value_or(t);
    });
    if (!wrapped)
        statusBar()->showMessage(tr("No block comment to remove"), 3000);
}

// One indent unit per line: a tab, or tab-width spaces when the editor
// inserts spaces. The selected lines, or the current line.
void MainWindow::increaseIndent()
{
    Editor *editor = currentEditor();
    if (!editor)
        return;
    const QString unit = editor->insertSpaces() ? QString(editor->tabWidth(), QLatin1Char(' '))
                                                : QStringLiteral("\t");
    transformCurrent(TransformScope::LinesOrCurrentLine,
                     [unit](const QString &t) { return TextTransforms::indentLines(t, unit); });
}

void MainWindow::decreaseIndent()
{
    Editor *editor = currentEditor();
    if (!editor)
        return;
    const int width = editor->tabWidth();
    transformCurrent(TransformScope::LinesOrCurrentLine,
                     [width](const QString &t) { return TextTransforms::unindentLines(t, width); });
}

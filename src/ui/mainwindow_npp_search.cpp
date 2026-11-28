// Notepad++ Search additions: Select and Find Next/Previous, the matching-brace
// commands, bookmarked-line operations, and the Find dialog's In selection and
// Replace All in All Opened Documents. Everything works on the in-memory
// buffers; nothing here saves a file.
#include "mainwindow.h"
#include "editor.h"
#include "searchdialog.h"
#include "core/document.h"
#include "core/searchengine.h"
#include "core/texttransforms.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QStatusBar>

namespace {

// The editor's bookmarks as 0-based indices of lines that exist.
QSet<int> markedLineIndexes(const Editor *editor)
{
    const int lineCount = int(TextTransforms::splitLines(editor->rawText()).lines.size());
    QSet<int> lines;
    for (int line : editor->bookmarks()) {
        if (line >= 1 && line <= lineCount)
            lines.insert(line - 1);
    }
    return lines;
}

} // namespace

// Shortcuts as in Notepad++: Ctrl+F3, Ctrl+Shift+F3, Ctrl+B, Ctrl+Alt+B.
// Ctrl+B was a window QShortcut; it moves to the menu item, since one key on
// two shortcuts is ambiguous and fires neither.
void MainWindow::setupNppSearchMenu()
{
    m_searchMenu->addSeparator();
    m_searchMenu->addAction(tr("Select and Find Next"), this, [this]() { selectAndFind(false); },
                            QKeySequence(Qt::CTRL | Qt::Key_F3));
    m_searchMenu->addAction(tr("Select and Find Previous"), this, [this]() { selectAndFind(true); },
                            QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F3));
    m_searchMenu->addAction(tr("Go to Matching Brace"), this, [this]() {
        if (Editor *e = currentEditor()) e->jumpToMatchingBracket();
    }, QKeySequence(Qt::CTRL | Qt::Key_B));
    m_searchMenu->addAction(tr("Select All between Matching Braces"), this, [this]() {
        if (Editor *e = currentEditor()) e->selectBetweenMatchingBraces();
    }, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_B));

    QMenu *bookmarks = m_searchMenu->addMenu(tr("Bookmark"));
    bookmarks->addAction(tr("Cut Bookmarked Lines"), this, &MainWindow::cutBookmarkedLines);
    bookmarks->addAction(tr("Copy Bookmarked Lines"), this, [this]() { copyBookmarkedLines(); });
    bookmarks->addAction(tr("Paste to (Replace) Bookmarked Lines"), this, &MainWindow::pasteToBookmarkedLines);
    bookmarks->addAction(tr("Remove Bookmarked Lines"), this, &MainWindow::removeBookmarkedLines);
    bookmarks->addAction(tr("Remove Unmarked Lines"), this, &MainWindow::removeUnmarkedLines);
    bookmarks->addAction(tr("Inverse Bookmark"), this, &MainWindow::inverseBookmark);
}

// The selection, or the word under the cursor, becomes the search text (so
// F3 continues with it) and the next or previous exact match is selected.
void MainWindow::selectAndFind(bool backward)
{
    Editor *editor = currentEditor();
    if (!editor)
        return;
    const QString word = editor->selectWordIfNoSelection();
    if (word.isEmpty())
        return;
    ensureSearchDialog();
    m_searchDialog->setSearchText(word);
    QTextDocument::FindFlags flags = QTextDocument::FindCaseSensitively;
    if (backward)
        flags |= QTextDocument::FindBackward;
    if (!editor->findNext(word, flags))
        statusBar()->showMessage(tr("Can't find \"%1\"").arg(word), 3000);
}

bool MainWindow::copyBookmarkedLines()
{
    Editor *editor = currentEditor();
    if (!editor)
        return false;
    const QSet<int> marked = markedLineIndexes(editor);
    if (marked.isEmpty()) {
        showNotification(tr("There are no bookmarked lines."));
        return false;
    }
    QApplication::clipboard()->setText(TextTransforms::copyLines(editor->rawText(), marked));
    return true;
}

void MainWindow::cutBookmarkedLines()
{
    Editor *editor = currentEditor();
    if (!editor)
        return;
    if (editor->isReadOnly()) {
        notifyReadOnly(editor);
        return;
    }
    if (copyBookmarkedLines())
        removeBookmarkedLines();
}

void MainWindow::removeBookmarkedLines()
{
    Editor *editor = currentEditor();
    if (!editor)
        return;
    const QSet<int> marked = markedLineIndexes(editor);
    if (transformCurrent(TransformScope::Document, [marked](const QString &t) {
            return TextTransforms::keepLines(t, marked, false);
        })) {
        editor->setBookmarks({});
    }
}

// What is left is exactly the marked lines, so they stay marked.
void MainWindow::removeUnmarkedLines()
{
    Editor *editor = currentEditor();
    if (!editor)
        return;
    const QSet<int> marked = markedLineIndexes(editor);
    if (transformCurrent(TransformScope::Document, [marked](const QString &t) {
            return TextTransforms::keepLines(t, marked, true);
        })) {
        QSet<int> kept;
        for (int line = 1; line <= marked.size(); ++line)
            kept.insert(line);
        editor->setBookmarks(kept);
    }
}

// Each bookmarked line becomes the clipboard's lines; the mark moves to the
// first of them.
void MainWindow::pasteToBookmarkedLines()
{
    Editor *editor = currentEditor();
    if (!editor)
        return;
    const QString clip = QApplication::clipboard()->text();
    const QSet<int> marked = markedLineIndexes(editor);
    if (!transformCurrent(TransformScope::Document, [marked, clip](const QString &t) {
            return TextTransforms::replaceLines(t, marked, clip);
        })) {
        return;
    }
    QList<int> order = marked.values();
    std::sort(order.begin(), order.end());
    const int grow = int(TextTransforms::splitLines(clip).lines.size()) - 1;
    QSet<int> moved;
    for (int i = 0; i < order.size(); ++i)
        moved.insert(order.at(i) + i * grow + 1);
    editor->setBookmarks(moved);
}

void MainWindow::inverseBookmark()
{
    Editor *editor = currentEditor();
    if (!editor)
        return;
    const QSet<int> current = editor->bookmarks();
    QSet<int> inverted;
    for (int line = 1; line <= editor->blockCount(); ++line) {
        if (!current.contains(line))
            inverted.insert(line);
    }
    editor->setBookmarks(inverted);
}

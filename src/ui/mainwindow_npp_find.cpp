// The Find dialog's wiring, with the Notepad++ "In selection" option for
// Replace All, Count and Mark, and Replace All in All Opened Documents.
// Replacements change the in-memory buffers only: a document becomes
// modified and is never saved here.
#include "mainwindow.h"
#include "editor.h"
#include "searchdialog.h"
#include "core/document.h"
#include "core/searchengine.h"

#include <QStatusBar>

void MainWindow::ensureSearchDialog()
{
    if (m_searchDialog)
        return;
    m_searchDialog = new SearchDialog(this);
    connect(m_searchDialog, &SearchDialog::findNext, this, &MainWindow::findNext);
    connect(m_searchDialog, &SearchDialog::findPrevious, this, &MainWindow::findPrevious);
    connect(m_searchDialog, &SearchDialog::replaceOne, this, [this]() {
        Editor *e = currentEditor();
        if (!e || !m_searchDialog) return;
        if (!e->selectedText().isEmpty()) {
            e->insertPlainText(m_searchDialog->replaceText());
        }
        findNext();
    });
    connect(m_searchDialog, &SearchDialog::replaceAll, this, &MainWindow::replaceAllInCurrent);
    connect(m_searchDialog, &SearchDialog::replaceAllInAllOpen, this, &MainWindow::replaceAllInAllOpen);
    connect(m_searchDialog, &SearchDialog::countRequested, this, &MainWindow::onCountRequested);
    connect(m_searchDialog, &SearchDialog::findAllInCurrent, this, &MainWindow::onFindAllInCurrent);
    connect(m_searchDialog, &SearchDialog::findAllInAllOpen, this, &MainWindow::onFindAllInAllOpen);
    connect(m_searchDialog, &SearchDialog::findInFiles, this, &MainWindow::onFindInFiles);
    connect(m_searchDialog, &SearchDialog::markAll, this, &MainWindow::markAllFromDialog);
    connect(m_searchDialog, &SearchDialog::clearMarks, this, [this]() {
        Editor *e = currentEditor();
        if (e) e->clearMarks();
    });
}

// A one-line selection becomes the search text. A multi-line one turns on
// In selection instead, as Notepad++ does.
void MainWindow::fillSearchFromSelection()
{
    Editor *e = currentEditor();
    if (!e || !e->textCursor().hasSelection())
        return;
    const QString selected = e->selectedText();
    if (selected.contains(QChar::ParagraphSeparator))
        m_searchDialog->setInSelection(true);
    else
        m_searchDialog->setSearchText(selected);
}

// The text a search covers: the selection when In selection is on (nullopt,
// with a notice, when nothing is selected), otherwise the whole document.
std::optional<QString> MainWindow::searchScopeText(Editor *e)
{
    if (!m_searchDialog->inSelection())
        return e->rawText();
    const QTextCursor cursor = e->textCursor();
    if (!cursor.hasSelection()) {
        showNotification(tr("In selection is on, but nothing is selected."));
        return std::nullopt;
    }
    return e->textInRange(cursor.selectionStart(), cursor.selectionEnd());
}

// Replaces every match within scope as one undo step; returns the count.
int MainWindow::replaceAllIn(Editor *e, TransformScope scope)
{
    const QString pattern = m_searchDialog->searchText();
    const QString replacement = m_searchDialog->replaceText();
    const SearchEngine::Options options = m_searchDialog->searchOptions();
    int count = 0;
    e->applyTextTransform(scope, [&](const QString &text) {
        SearchEngine engine;
        count = engine.matchCount(text, pattern, options);
        return count ? engine.replaceInText(text, pattern, replacement, options) : text;
    });
    return count;
}

void MainWindow::replaceAllInCurrent()
{
    Editor *e = currentEditor();
    if (!e || !m_searchDialog || !searchScopeText(e))
        return;
    if (e->isReadOnly()) {
        notifyReadOnly(e);
        return;
    }
    const int count = replaceAllIn(e, m_searchDialog->inSelection() ? TransformScope::Selection
                                                                    : TransformScope::Document);
    statusBar()->showMessage(tr("Replace All: %n occurrence(s) replaced", nullptr, count), 5000);
}

// Every open document once (a document can be shown in both panes);
// read-only ones are skipped and counted.
void MainWindow::replaceAllInAllOpen()
{
    if (!m_searchDialog)
        return;
    QSet<Document *> seen;
    int total = 0, documents = 0, readOnly = 0;
    for (Editor *e : allEditors()) {
        if (seen.contains(e->document()))
            continue;
        seen.insert(e->document());
        if (e->isReadOnly()) {
            ++readOnly;
            continue;
        }
        const int count = replaceAllIn(e, TransformScope::Document);
        total += count;
        documents += count ? 1 : 0;
    }
    statusBar()->showMessage(tr("Replace All in All Opened Documents: %1 occurrence(s) in %2 document(s)")
                                 .arg(total).arg(documents), 5000);
    if (readOnly)
        showNotification(tr("%n read-only document(s) were not changed.", nullptr, readOnly));
}

void MainWindow::markAllFromDialog()
{
    Editor *e = currentEditor();
    if (!e || !m_searchDialog || !searchScopeText(e))
        return;
    const QTextCursor cursor = e->textCursor();
    if (m_searchDialog->inSelection())
        e->markAll(m_searchDialog->searchText(), m_searchDialog->searchOptions(),
                   cursor.selectionStart(), cursor.selectionEnd());
    else
        e->markAll(m_searchDialog->searchText(), m_searchDialog->searchOptions());
}

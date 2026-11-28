// Notepad++-parity Edit commands. Menu wording and default shortcuts follow
// Notepad++ wherever they do not clash with an existing PrimeEdit shortcut.
// Every text command is a pure TextTransforms function applied through
// Editor::applyTextTransform, so it is one undo step and marks the file
// modified like any other edit.
#include "mainwindow.h"
#include "editor.h"
#include "core/document.h"
#include "core/texttransforms.h"

#include <QAction>
#include <QActionGroup>
#include <QMenu>

QAction *MainWindow::addTransformAction(QMenu *menu, const QString &text, TransformScope scope,
                                        const TextTransform &fn, const QKeySequence &shortcut)
{
    QAction *action = menu->addAction(text, this, [this, scope, fn]() { transformCurrent(scope, fn); });
    if (!shortcut.isEmpty())
        action->setShortcut(shortcut);
    return action;
}

bool MainWindow::transformCurrent(TransformScope scope, const TextTransform &fn)
{
    Editor *editor = currentEditor();
    if (!editor)
        return false;
    if (editor->isReadOnly()) {
        showNotification(tr("'%1' is read-only, so it was not changed.")
                             .arg(editor->document()->displayName()));
        return false;
    }
    return editor->applyTextTransform(scope, fn);
}

void MainWindow::removeEmptyLines()
{
    transformCurrent(TransformScope::LinesOrDocument, TextTransforms::removeEmptyLines);
}

void MainWindow::removeEmptyLinesWithBlanks()
{
    transformCurrent(TransformScope::LinesOrDocument, TextTransforms::removeBlankLines);
}

void MainWindow::removeConsecutiveDuplicateLines()
{
    transformCurrent(TransformScope::LinesOrDocument, TextTransforms::removeConsecutiveDuplicateLines);
}

void MainWindow::trimTrailingSpace()
{
    transformCurrent(TransformScope::LinesOrDocument, TextTransforms::trimTrailing);
}

void MainWindow::trimLeadingWhitespace()
{
    transformCurrent(TransformScope::LinesOrDocument, TextTransforms::trimLeading);
}

void MainWindow::trimLeadingAndTrailing()
{
    transformCurrent(TransformScope::LinesOrDocument, TextTransforms::trimBoth);
}

void MainWindow::eolToSpace()
{
    transformCurrent(TransformScope::LinesOrDocument, TextTransforms::eolToSpace);
}

// Notepad++ "Remove Unnecessary Blank and EOL": trim both ends, then EOL to space
void MainWindow::removeUnnecessaryBlanks()
{
    transformCurrent(TransformScope::LinesOrDocument, TextTransforms::trimBothAndEolToSpace);
}

void MainWindow::setupLineOperationsMenu(QMenu *menu)
{
    menu->addSeparator();
    menu->addAction(tr("&Remove Duplicate Lines"), this, &MainWindow::removeDuplicateLines);
    menu->addAction(tr("Remove Consecutive Duplicate Lines"), this,
                    &MainWindow::removeConsecutiveDuplicateLines);
    menu->addAction(tr("&Join Lines"), this, &MainWindow::joinLines);
    menu->addAction(tr("Remove Empty Lines"), this, &MainWindow::removeEmptyLines);
    menu->addAction(tr("Remove Empty Lines (Containing Blank characters)"), this,
                    &MainWindow::removeEmptyLinesWithBlanks);
    menu->addSeparator();
    menu->addAction(tr("&Sort Lines (Ascending)"), this, &MainWindow::sortLinesAscending);
    menu->addAction(tr("Sort Lines (Descending)"), this, &MainWindow::sortLinesDescending);
}

void MainWindow::setupNppEditMenus()
{
    QMenu *caseMenu = m_editMenu->addMenu(tr("&Convert Case"));
    caseMenu->addAction(tr("&UPPERCASE"), this, &MainWindow::toUpperCase);
    caseMenu->addAction(tr("&lowercase"), this, &MainWindow::toLowerCase);
    caseMenu->addAction(tr("&Title Case"), this, &MainWindow::toTitleCase);

    QMenu *blankMenu = m_editMenu->addMenu(tr("&Blank Operations"));
    blankMenu->addAction(tr("Trim Trailing Space"), this, &MainWindow::trimTrailingSpace);
    blankMenu->addAction(tr("Trim Leading Space"), this, &MainWindow::trimLeadingWhitespace);
    blankMenu->addAction(tr("Trim Leading and Trailing Space"), this,
                         &MainWindow::trimLeadingAndTrailing);
    blankMenu->addAction(tr("EOL to Space"), this, &MainWindow::eolToSpace);
    blankMenu->addAction(tr("Remove Unnecessary Blank and EOL"), this,
                         &MainWindow::removeUnnecessaryBlanks);

    QMenu *eolMenu = m_editMenu->addMenu(tr("EOL Conversion"));
    QActionGroup *eolGroup = new QActionGroup(eolMenu);
    const auto addEol = [&](const QString &text, void (MainWindow::*slot)(), Document::LineEnding ending) {
        QAction *action = eolMenu->addAction(text, this, slot);
        action->setCheckable(true);
        action->setData(int(ending));
        eolGroup->addAction(action);
    };
    addEol(tr("Windows (CR LF)"), &MainWindow::convertToWindows, Document::Windows);
    addEol(tr("Unix (LF)"), &MainWindow::convertToUnix, Document::Unix);
    addEol(tr("Macintosh (CR)"), &MainWindow::convertToMac, Document::ClassicMac);
    connect(eolMenu, &QMenu::aboutToShow, this, [this, eolMenu]() { syncEolMenu(eolMenu); });
}

// Checks the current document's line ending; a document that cannot be
// converted (partial view, read-only) shows the items disabled.
void MainWindow::syncEolMenu(QMenu *menu)
{
    Editor *editor = currentEditor();
    const Document *doc = editor ? editor->document() : nullptr;
    for (QAction *action : menu->actions()) {
        action->setEnabled(doc && isConvertible(doc));
        action->setChecked(doc && action->data().toInt() == int(doc->lineEnding()));
    }
}

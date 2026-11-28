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
#include <QRandomGenerator>

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

// Insert Blank Line Above/Below Current keep the Notepad++ shortcuts
// Ctrl+Alt+Enter and Ctrl+Alt+Shift+Enter; neither was taken.
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
    const auto insertBlank = [this](bool above) {
        Editor *editor = currentEditor();
        if (editor) editor->insertBlankLine(above);
    };
    menu->addAction(tr("Insert Blank Line Above Current"), this, [insertBlank]() { insertBlank(true); },
                    QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Return));
    menu->addAction(tr("Insert Blank Line Below Current"), this, [insertBlank]() { insertBlank(false); },
                    QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_Return));
    menu->addSeparator();
    setupSortMenu(menu);
}

void MainWindow::setupSortMenu(QMenu *menu)
{
    using namespace TextTransforms;
    const TransformScope lines = TransformScope::LinesOrDocument;
    const auto pair = [this, menu, lines](const QString &ascending, const QString &descending,
                                          const std::function<QString(const QString &, SortOrder)> &sort) {
        addTransformAction(menu, ascending, lines,
                           [sort](const QString &t) { return sort(t, SortOrder::Ascending); });
        addTransformAction(menu, descending, lines,
                           [sort](const QString &t) { return sort(t, SortOrder::Descending); });
    };
    menu->addAction(tr("Sort Lines Lexicographically Ascending"), this, &MainWindow::sortLinesAscending);
    menu->addAction(tr("Sort Lines Lexicographically Descending"), this, &MainWindow::sortLinesDescending);
    pair(tr("Sort Lines Lex. Ascending Ignoring Case"), tr("Sort Lines Lex. Descending Ignoring Case"),
         [](const QString &t, SortOrder o) { return sortLexicographic(t, o, Qt::CaseInsensitive); });
    pair(tr("Sort Lines In Locale Order Ascending"), tr("Sort Lines In Locale Order Descending"),
         [](const QString &t, SortOrder o) { return sortLocale(t, o, QLocale::system()); });
    pair(tr("Sort Lines As Integers Ascending"), tr("Sort Lines As Integers Descending"), sortIntegers);
    pair(tr("Sort Lines As Decimals (Comma) Ascending"), tr("Sort Lines As Decimals (Comma) Descending"),
         [](const QString &t, SortOrder o) { return sortDecimals(t, o, QLatin1Char(',')); });
    pair(tr("Sort Lines As Decimals (Dot) Ascending"), tr("Sort Lines As Decimals (Dot) Descending"),
         [](const QString &t, SortOrder o) { return sortDecimals(t, o, QLatin1Char('.')); });
    pair(tr("Sort Lines By Length Ascending"), tr("Sort Lines By Length Descending"), sortByLength);
    menu->addSeparator();
    addTransformAction(menu, tr("Reverse Line Order"), lines, reverseLines);
    addTransformAction(menu, tr("Randomize Line Order"), lines, [](const QString &t) {
        return shuffleLines(t, QRandomGenerator::global()->generate());
    });
}

void MainWindow::sortLinesAscending()
{
    transformCurrent(TransformScope::LinesOrDocument, [](const QString &t) {
        return TextTransforms::sortLexicographic(t, TextTransforms::SortOrder::Ascending, Qt::CaseSensitive);
    });
}

void MainWindow::sortLinesDescending()
{
    transformCurrent(TransformScope::LinesOrDocument, [](const QString &t) {
        return TextTransforms::sortLexicographic(t, TextTransforms::SortOrder::Descending, Qt::CaseSensitive);
    });
}

// TAB/space conversions use the current editor's tab width.
void MainWindow::tabToSpace()
{
    Editor *editor = currentEditor();
    const int width = editor ? editor->tabWidth() : 4;
    transformCurrent(TransformScope::LinesOrDocument,
                     [width](const QString &t) { return TextTransforms::tabsToSpaces(t, width); });
}

void MainWindow::spaceToTabAll()
{
    Editor *editor = currentEditor();
    const int width = editor ? editor->tabWidth() : 4;
    transformCurrent(TransformScope::LinesOrDocument,
                     [width](const QString &t) { return TextTransforms::spacesToTabs(t, width, false); });
}

void MainWindow::spaceToTabLeading()
{
    Editor *editor = currentEditor();
    const int width = editor ? editor->tabWidth() : 4;
    transformCurrent(TransformScope::LinesOrDocument,
                     [width](const QString &t) { return TextTransforms::spacesToTabs(t, width, true); });
}

// Notepad++ shortcuts: Alt+U, Alt+Shift+U, Ctrl+Alt+U, Ctrl+Alt+Shift+U,
// Ctrl+Alt+I, Ctrl+Alt+Shift+I. None is taken in PrimeEdit. Proper Case
// replaces the old Title Case item, which did the same with fewer rules.
void MainWindow::setupCaseMenu()
{
    const TransformScope sel = TransformScope::Selection;
    QMenu *caseMenu = m_editMenu->addMenu(tr("&Convert Case"));
    caseMenu->addAction(tr("&UPPERCASE"), this, &MainWindow::toUpperCase);
    caseMenu->addAction(tr("&lowercase"), this, &MainWindow::toLowerCase);
    addTransformAction(caseMenu, tr("&Proper Case"), sel,
                       [](const QString &t) { return TextTransforms::properCase(t, false); },
                       QKeySequence(Qt::ALT | Qt::Key_U));
    addTransformAction(caseMenu, tr("Proper Case (blend)"), sel,
                       [](const QString &t) { return TextTransforms::properCase(t, true); },
                       QKeySequence(Qt::ALT | Qt::SHIFT | Qt::Key_U));
    addTransformAction(caseMenu, tr("&Sentence case"), sel,
                       [](const QString &t) { return TextTransforms::sentenceCase(t, false); },
                       QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_U));
    addTransformAction(caseMenu, tr("Sentence case (blend)"), sel,
                       [](const QString &t) { return TextTransforms::sentenceCase(t, true); },
                       QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_U));
    addTransformAction(caseMenu, tr("&iNVERT cASE"), sel, TextTransforms::invertCase,
                       QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_I));
    addTransformAction(caseMenu, tr("ran&DOM cASE"), sel,
                       [](const QString &t) {
                           return TextTransforms::randomCase(t, QRandomGenerator::global()->generate());
                       },
                       QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_I));
}

void MainWindow::setupNppEditMenus()
{
    setupCaseMenu();
    setupBlankOperationsMenu();
    setupEolMenu();
    setupCommentAndIndentMenus();
    setupInsertAndClipboardMenus();
}

void MainWindow::setupBlankOperationsMenu()
{
    QMenu *blankMenu = m_editMenu->addMenu(tr("&Blank Operations"));
    blankMenu->addAction(tr("Trim Trailing Space"), this, &MainWindow::trimTrailingSpace);
    blankMenu->addAction(tr("Trim Leading Space"), this, &MainWindow::trimLeadingWhitespace);
    blankMenu->addAction(tr("Trim Leading and Trailing Space"), this,
                         &MainWindow::trimLeadingAndTrailing);
    blankMenu->addAction(tr("EOL to Space"), this, &MainWindow::eolToSpace);
    blankMenu->addAction(tr("Remove Unnecessary Blank and EOL"), this,
                         &MainWindow::removeUnnecessaryBlanks);
    blankMenu->addSeparator();
    blankMenu->addAction(tr("TAB to Space"), this, &MainWindow::tabToSpace);
    blankMenu->addAction(tr("Space to TAB (All)"), this, &MainWindow::spaceToTabAll);
    blankMenu->addAction(tr("Space to TAB (Leading)"), this, &MainWindow::spaceToTabLeading);
}

void MainWindow::setupEolMenu()
{
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

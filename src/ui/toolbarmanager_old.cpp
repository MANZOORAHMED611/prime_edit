#include "toolbarmanager.h"
#include "mainwindow.h"
#include <QStyle>
#include <QApplication>

ToolbarManager::ToolbarManager(MainWindow *mainWindow, QObject *parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
    , m_fileToolBar(nullptr)
    , m_editToolBar(nullptr)
    , m_searchToolBar(nullptr)
    , m_viewToolBar(nullptr)
    , m_formatToolBar(nullptr)
    , m_macroToolBar(nullptr)
{
}

void ToolbarManager::createToolbars()
{
    createFileToolbar();
    createEditToolbar();
    createSearchToolbar();
    createViewToolbar();
    createFormatToolbar();
    createMacroToolbar();
}

QAction* ToolbarManager::createAction(const QString &name, const QString &text,
                                     const QString &tooltip, const QString &shortcut,
                                     const char *slot, const QString &iconName)
{
    QAction *action = new QAction(text, m_mainWindow);
    action->setToolTip(tooltip);

    if (!shortcut.isEmpty()) {
        action->setShortcut(QKeySequence(shortcut));
    }

    if (!iconName.isEmpty()) {
        // Try to load icon from resources, fall back to style icons
        QIcon icon(iconName);
        if (!icon.isNull()) {
            action->setIcon(icon);
        } else {
            // Use Qt standard icons as fallback
            action->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_FileIcon));
        }
    }

    if (slot) {
        connect(action, SIGNAL(triggered()), m_mainWindow, slot);
    }

    m_actions[name] = action;
    return action;
}

void ToolbarManager::createFileToolbar()
{
    m_fileToolBar = m_mainWindow->addToolBar(tr("File"));
    m_fileToolBar->setObjectName("FileToolBar");
    m_fileToolBar->setIconSize(QSize(20, 20));
    m_fileToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    // New
    QAction *newAction = createAction("new", tr("New"), tr("New File (Ctrl+N)"),
                                     "Ctrl+N", SLOT(newFile()));
    newAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_FileIcon));
    m_fileToolBar->addAction(newAction);

    // Open
    QAction *openAction = createAction("open", tr("Open"), tr("Open File (Ctrl+O)"),
                                      "Ctrl+O", SLOT(openFile()));
    openAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_DirOpenIcon));
    m_fileToolBar->addAction(openAction);

    // Save
    QAction *saveAction = createAction("save", tr("Save"), tr("Save File (Ctrl+S)"),
                                      "Ctrl+S", SLOT(saveFile()));
    saveAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_fileToolBar->addAction(saveAction);

    // Save All
    QAction *saveAllAction = createAction("saveAll", tr("Save All"), tr("Save All Files (Ctrl+Shift+S)"),
                                         "Ctrl+Shift+S", SLOT(saveAllFiles()));
    saveAllAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_fileToolBar->addAction(saveAllAction);

    m_fileToolBar->addSeparator();

    // Close
    QAction *closeAction = createAction("close", tr("Close"), tr("Close File (Ctrl+W)"),
                                       "Ctrl+W", SLOT(closeFile()));
    closeAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_DialogCloseButton));
    m_fileToolBar->addAction(closeAction);

    // Close All
    QAction *closeAllAction = createAction("closeAll", tr("Close All"), tr("Close All Files"),
                                          "", SLOT(closeAllFiles()));
    closeAllAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_DialogDiscardButton));
    m_fileToolBar->addAction(closeAllAction);

    m_fileToolBar->addSeparator();

    // Print (placeholder - not implemented yet)
    QAction *printAction = createAction("print", tr("Print"), tr("Print (Ctrl+P)"),
                                       "Ctrl+P", nullptr);
    printAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_DialogYesButton));
    printAction->setEnabled(false);
    m_fileToolBar->addAction(printAction);
}

void ToolbarManager::createEditToolbar()
{
    m_editToolBar = m_mainWindow->addToolBar(tr("Edit"));
    m_editToolBar->setObjectName("EditToolBar");
    m_editToolBar->setIconSize(QSize(24, 24));

    // Undo
    QAction *undoAction = createAction("undo", tr("Undo"), tr("Undo (Ctrl+Z)"),
                                      "Ctrl+Z", SLOT(undo()));
    undoAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_ArrowBack));
    m_editToolBar->addAction(undoAction);

    // Redo
    QAction *redoAction = createAction("redo", tr("Redo"), tr("Redo (Ctrl+Y)"),
                                      "Ctrl+Y", SLOT(redo()));
    redoAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_ArrowForward));
    m_editToolBar->addAction(redoAction);

    m_editToolBar->addSeparator();

    // Cut
    QAction *cutAction = createAction("cut", tr("Cut"), tr("Cut (Ctrl+X)"),
                                     "Ctrl+X", SLOT(cut()));
    cutAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_DialogDiscardButton));
    m_editToolBar->addAction(cutAction);

    // Copy
    QAction *copyAction = createAction("copy", tr("Copy"), tr("Copy (Ctrl+C)"),
                                      "Ctrl+C", SLOT(copy()));
    copyAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    m_editToolBar->addAction(copyAction);

    // Paste
    QAction *pasteAction = createAction("paste", tr("Paste"), tr("Paste (Ctrl+V)"),
                                       "Ctrl+V", SLOT(paste()));
    pasteAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_DialogOkButton));
    m_editToolBar->addAction(pasteAction);

    m_editToolBar->addSeparator();

    // Select All
    QAction *selectAllAction = createAction("selectAll", tr("Select All"), tr("Select All (Ctrl+A)"),
                                           "Ctrl+A", SLOT(selectAll()));
    m_editToolBar->addAction(selectAllAction);

    m_editToolBar->addSeparator();

    // Duplicate Line
    QAction *dupLineAction = createAction("duplicateLine", tr("Duplicate Line"),
                                         tr("Duplicate Line (Ctrl+D)"),
                                         "Ctrl+D", SLOT(duplicateLine()));
    m_editToolBar->addAction(dupLineAction);

    // Delete Line
    QAction *delLineAction = createAction("deleteLine", tr("Delete Line"),
                                         tr("Delete Line (Ctrl+Shift+L)"),
                                         "Ctrl+Shift+L", SLOT(deleteLine()));
    m_editToolBar->addAction(delLineAction);

    // Move Line Up
    QAction *moveUpAction = createAction("moveLineUp", tr("Move Line Up"),
                                        tr("Move Line Up (Ctrl+Shift+Up)"),
                                        "Ctrl+Shift+Up", SLOT(moveLineUp()));
    m_editToolBar->addAction(moveUpAction);

    // Move Line Down
    QAction *moveDownAction = createAction("moveLineDown", tr("Move Line Down"),
                                          tr("Move Line Down (Ctrl+Shift+Down)"),
                                          "Ctrl+Shift+Down", SLOT(moveLineDown()));
    m_editToolBar->addAction(moveDownAction);

    m_editToolBar->addSeparator();

    // Toggle Comment
    QAction *toggleCommentAction = createAction("toggleComment", tr("Toggle Comment"),
                                               tr("Toggle Comment (Ctrl+/)"),
                                               "Ctrl+/", SLOT(toggleComment()));
    m_editToolBar->addAction(toggleCommentAction);

    // Block Comment
    QAction *blockCommentAction = createAction("blockComment", tr("Block Comment"),
                                              tr("Block Comment (Ctrl+Shift+/)"),
                                              "Ctrl+Shift+/", SLOT(blockComment()));
    m_editToolBar->addAction(blockCommentAction);
}

void ToolbarManager::createSearchToolbar()
{
    m_searchToolBar = m_mainWindow->addToolBar(tr("Search"));
    m_searchToolBar->setObjectName("SearchToolBar");
    m_searchToolBar->setIconSize(QSize(24, 24));

    // Find
    QAction *findAction = createAction("find", tr("Find"), tr("Find (Ctrl+F)"),
                                      "Ctrl+F", SLOT(find()));
    findAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_FileDialogContentsView));
    m_searchToolBar->addAction(findAction);

    // Replace
    QAction *replaceAction = createAction("replace", tr("Replace"), tr("Replace (Ctrl+H)"),
                                         "Ctrl+H", SLOT(replace()));
    replaceAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_BrowserReload));
    m_searchToolBar->addAction(replaceAction);

    m_searchToolBar->addSeparator();

    // Find Next
    QAction *findNextAction = createAction("findNext", tr("Find Next"), tr("Find Next (F3)"),
                                          "F3", SLOT(findNext()));
    findNextAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_ArrowDown));
    m_searchToolBar->addAction(findNextAction);

    // Find Previous
    QAction *findPrevAction = createAction("findPrevious", tr("Find Previous"),
                                          tr("Find Previous (Shift+F3)"),
                                          "Shift+F3", SLOT(findPrevious()));
    findPrevAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_ArrowUp));
    m_searchToolBar->addAction(findPrevAction);

    m_searchToolBar->addSeparator();

    // Go to Line
    QAction *gotoLineAction = createAction("gotoLine", tr("Go to Line"), tr("Go to Line (Ctrl+G)"),
                                          "Ctrl+G", SLOT(goToLineDialog()));
    m_searchToolBar->addAction(gotoLineAction);

    m_searchToolBar->addSeparator();

    // Toggle Bookmark
    QAction *toggleBookmarkAction = createAction("toggleBookmark", tr("Toggle Bookmark"),
                                                tr("Toggle Bookmark (Ctrl+F2)"),
                                                "Ctrl+F2", SLOT(toggleBookmark()));
    m_searchToolBar->addAction(toggleBookmarkAction);

    // Next Bookmark
    QAction *nextBookmarkAction = createAction("nextBookmark", tr("Next Bookmark"),
                                              tr("Next Bookmark (F2)"),
                                              "F2", SLOT(nextBookmark()));
    m_searchToolBar->addAction(nextBookmarkAction);

    // Previous Bookmark
    QAction *prevBookmarkAction = createAction("previousBookmark", tr("Previous Bookmark"),
                                              tr("Previous Bookmark (Shift+F2)"),
                                              "Shift+F2", SLOT(previousBookmark()));
    m_searchToolBar->addAction(prevBookmarkAction);

    // Clear All Bookmarks
    QAction *clearBookmarksAction = createAction("clearBookmarks", tr("Clear Bookmarks"),
                                                tr("Clear All Bookmarks"),
                                                "", SLOT(clearAllBookmarks()));
    m_searchToolBar->addAction(clearBookmarksAction);
}

void ToolbarManager::createViewToolbar()
{
    m_viewToolBar = m_mainWindow->addToolBar(tr("View"));
    m_viewToolBar->setObjectName("ViewToolBar");
    m_viewToolBar->setIconSize(QSize(24, 24));

    // Zoom In
    QAction *zoomInAction = createAction("zoomIn", tr("Zoom In"), tr("Zoom In (Ctrl++)"),
                                        "Ctrl++", SLOT(zoomIn()));
    zoomInAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_DialogYesButton));
    m_viewToolBar->addAction(zoomInAction);

    // Zoom Out
    QAction *zoomOutAction = createAction("zoomOut", tr("Zoom Out"), tr("Zoom Out (Ctrl+-)"),
                                         "Ctrl+-", SLOT(zoomOut()));
    zoomOutAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_DialogNoButton));
    m_viewToolBar->addAction(zoomOutAction);

    // Reset Zoom
    QAction *resetZoomAction = createAction("resetZoom", tr("Reset Zoom"), tr("Reset Zoom (Ctrl+0)"),
                                           "Ctrl+0", SLOT(resetZoom()));
    m_viewToolBar->addAction(resetZoomAction);

    m_viewToolBar->addSeparator();

    // Word Wrap
    QAction *wordWrapAction = createAction("wordWrap", tr("Word Wrap"), tr("Toggle Word Wrap"),
                                          "", SLOT(toggleWordWrap()));
    wordWrapAction->setCheckable(true);
    m_viewToolBar->addAction(wordWrapAction);

    // Show Whitespace
    QAction *whitespaceAction = createAction("showWhitespace", tr("Show Whitespace"),
                                            tr("Show Whitespace and TAB"),
                                            "", SLOT(toggleWhitespace()));
    whitespaceAction->setCheckable(true);
    m_viewToolBar->addAction(whitespaceAction);

    // Show EOL
    QAction *eolAction = createAction("showEOL", tr("Show EOL"), tr("Show End of Line"),
                                     "", SLOT(toggleEndOfLine()));
    eolAction->setCheckable(true);
    m_viewToolBar->addAction(eolAction);

    // Show Indent Guide
    QAction *indentAction = createAction("showIndent", tr("Show Indent"),
                                        tr("Show Indent Guide"),
                                        "", SLOT(toggleIndentGuide()));
    indentAction->setCheckable(true);
    m_viewToolBar->addAction(indentAction);

    m_viewToolBar->addSeparator();

    // Full Screen
    QAction *fullScreenAction = createAction("fullScreen", tr("Full Screen"),
                                            tr("Toggle Full Screen (F11)"),
                                            "F11", SLOT(toggleFullScreen()));
    fullScreenAction->setCheckable(true);
    m_viewToolBar->addAction(fullScreenAction);

    // Distraction Free
    QAction *distractionFreeAction = createAction("distractionFree", tr("Distraction Free"),
                                                 tr("Distraction Free Mode"),
                                                 "", SLOT(toggleDistractionFree()));
    distractionFreeAction->setCheckable(true);
    m_viewToolBar->addAction(distractionFreeAction);

    m_viewToolBar->addSeparator();

    // Fold All
    QAction *foldAllAction = createAction("foldAll", tr("Fold All"), tr("Fold All"),
                                         "", SLOT(foldAll()));
    m_viewToolBar->addAction(foldAllAction);

    // Unfold All
    QAction *unfoldAllAction = createAction("unfoldAll", tr("Unfold All"), tr("Unfold All"),
                                           "", SLOT(unfoldAll()));
    m_viewToolBar->addAction(unfoldAllAction);

    // Toggle Fold
    QAction *toggleFoldAction = createAction("toggleFold", tr("Toggle Fold"),
                                            tr("Toggle Current Fold"),
                                            "", SLOT(toggleFold()));
    m_viewToolBar->addAction(toggleFoldAction);
}

void ToolbarManager::createFormatToolbar()
{
    m_formatToolBar = m_mainWindow->addToolBar(tr("Format"));
    m_formatToolBar->setObjectName("FormatToolBar");
    m_formatToolBar->setIconSize(QSize(24, 24));

    // Uppercase
    QAction *upperAction = createAction("uppercase", tr("UPPERCASE"), tr("Convert to UPPERCASE"),
                                       "Ctrl+Shift+U", SLOT(toUpperCase()));
    m_formatToolBar->addAction(upperAction);

    // Lowercase
    QAction *lowerAction = createAction("lowercase", tr("lowercase"), tr("Convert to lowercase"),
                                       "Ctrl+U", SLOT(toLowerCase()));
    m_formatToolBar->addAction(lowerAction);

    // Title Case
    QAction *titleAction = createAction("titleCase", tr("Title Case"), tr("Convert to Title Case"),
                                       "", SLOT(toTitleCase()));
    m_formatToolBar->addAction(titleAction);

    // Sentence Case
    QAction *sentenceAction = createAction("sentenceCase", tr("Sentence case"),
                                          tr("Convert to Sentence case"),
                                          "", SLOT(toSentenceCase()));
    m_formatToolBar->addAction(sentenceAction);

    // Invert Case
    QAction *invertAction = createAction("invertCase", tr("iNVERT cASE"), tr("Invert Case"),
                                        "", SLOT(toInvertCase()));
    m_formatToolBar->addAction(invertAction);

    m_formatToolBar->addSeparator();

    // Trim Trailing
    QAction *trimTrailAction = createAction("trimTrailing", tr("Trim Trailing"),
                                           tr("Trim Trailing Whitespace"),
                                           "", SLOT(trimTrailingWhitespace()));
    m_formatToolBar->addAction(trimTrailAction);

    // Trim Leading
    QAction *trimLeadAction = createAction("trimLeading", tr("Trim Leading"),
                                          tr("Trim Leading Whitespace"),
                                          "", SLOT(trimLeadingWhitespace()));
    m_formatToolBar->addAction(trimLeadAction);

    // Trim Both
    QAction *trimBothAction = createAction("trimBoth", tr("Trim Both"),
                                          tr("Trim Leading and Trailing Whitespace"),
                                          "", SLOT(trimLeadingAndTrailing()));
    m_formatToolBar->addAction(trimBothAction);

    m_formatToolBar->addSeparator();

    // TAB to Space
    QAction *tabToSpaceAction = createAction("tabToSpace", tr("TAB to Space"),
                                            tr("Convert TABs to Spaces"),
                                            "", SLOT(tabToSpace()));
    m_formatToolBar->addAction(tabToSpaceAction);

    // Space to TAB
    QAction *spaceToTabAction = createAction("spaceToTab", tr("Space to TAB"),
                                            tr("Convert Spaces to TABs"),
                                            "", SLOT(spaceToTabAll()));
    m_formatToolBar->addAction(spaceToTabAction);

    m_formatToolBar->addSeparator();

    // Remove Empty Lines
    QAction *removeEmptyAction = createAction("removeEmpty", tr("Remove Empty"),
                                             tr("Remove Empty Lines"),
                                             "", SLOT(removeEmptyLines()));
    m_formatToolBar->addAction(removeEmptyAction);

    // Remove Duplicate Lines
    QAction *removeDupsAction = createAction("removeDuplicates", tr("Remove Duplicates"),
                                            tr("Remove Duplicate Lines"),
                                            "", SLOT(removeDuplicateLines()));
    m_formatToolBar->addAction(removeDupsAction);

    // Sort Ascending
    QAction *sortAscAction = createAction("sortAsc", tr("Sort A-Z"), tr("Sort Lines Ascending"),
                                         "", SLOT(sortLinesAscending()));
    m_formatToolBar->addAction(sortAscAction);

    // Sort Descending
    QAction *sortDescAction = createAction("sortDesc", tr("Sort Z-A"), tr("Sort Lines Descending"),
                                          "", SLOT(sortLinesDescending()));
    m_formatToolBar->addAction(sortDescAction);
}

void ToolbarManager::createMacroToolbar()
{
    m_macroToolBar = m_mainWindow->addToolBar(tr("Macro"));
    m_macroToolBar->setObjectName("MacroToolBar");
    m_macroToolBar->setIconSize(QSize(24, 24));

    // Start Recording
    QAction *startRecAction = createAction("startRecording", tr("Start Recording"),
                                          tr("Start Recording Macro (Ctrl+Shift+R)"),
                                          "Ctrl+Shift+R", SLOT(startRecordingMacro()));
    startRecAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_MediaPlay));
    m_macroToolBar->addAction(startRecAction);

    // Stop Recording
    QAction *stopRecAction = createAction("stopRecording", tr("Stop Recording"),
                                         tr("Stop Recording Macro"),
                                         "", SLOT(stopRecordingMacro()));
    stopRecAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_MediaStop));
    stopRecAction->setEnabled(false);
    m_macroToolBar->addAction(stopRecAction);

    m_macroToolBar->addSeparator();

    // Playback
    QAction *playbackAction = createAction("playback", tr("Playback"),
                                          tr("Playback Macro (Ctrl+Shift+P)"),
                                          "Ctrl+Shift+P", SLOT(playbackMacro()));
    playbackAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_MediaSeekForward));
    m_macroToolBar->addAction(playbackAction);

    m_macroToolBar->addSeparator();

    // Save Macro
    QAction *saveMacroAction = createAction("saveMacro", tr("Save Macro"),
                                           tr("Save Current Macro"),
                                           "", SLOT(saveMacro()));
    saveMacroAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_macroToolBar->addAction(saveMacroAction);

    // Load Macro
    QAction *loadMacroAction = createAction("loadMacro", tr("Load Macro"),
                                           tr("Load Macro from File"),
                                           "", SLOT(loadMacro()));
    loadMacroAction->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_DirOpenIcon));
    m_macroToolBar->addAction(loadMacroAction);
}

void ToolbarManager::updateToolbarState()
{
    // Update action states based on current editor state
    // This would be called when the editor state changes
}

QAction* ToolbarManager::action(const QString &name) const
{
    return m_actions.value(name, nullptr);
}

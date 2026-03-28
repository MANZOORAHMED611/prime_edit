#include "toolbarmanager.h"
#include "mainwindow.h"
#include <QStyle>
#include <QApplication>

ToolbarManager::ToolbarManager(MainWindow *mainWindow, QObject *parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
    , m_toolbar(nullptr)
{
}

void ToolbarManager::createToolbars()
{
    m_toolbar = m_mainWindow->addToolBar(tr("Main"));
    m_toolbar->setObjectName("MainToolBar");
    m_toolbar->setIconSize(QSize(20, 20));
    m_toolbar->setMovable(false);

    // Group 1 — File
    QAction *newAction = createAction("new", tr("New"), tr("New File (Ctrl+N)"),
                                     "Ctrl+N", SLOT(newFile()), ":/icons/toolbar/file-new.svg");
    m_toolbar->addAction(newAction);

    QAction *openAction = createAction("open", tr("Open"), tr("Open File (Ctrl+O)"),
                                      "Ctrl+O", SLOT(openFile()), ":/icons/toolbar/folder-open.svg");
    m_toolbar->addAction(openAction);

    QAction *saveAction = createAction("save", tr("Save"), tr("Save (Ctrl+S)"),
                                      "Ctrl+S", SLOT(saveFile()), ":/icons/toolbar/content-save.svg");
    m_toolbar->addAction(saveAction);

    QAction *saveAllAction = createAction("saveAll", tr("Save All"), tr("Save All (Ctrl+Shift+S)"),
                                         "Ctrl+Shift+S", SLOT(saveAllFiles()), ":/icons/toolbar/content-save-all.svg");
    m_toolbar->addAction(saveAllAction);

    QAction *closeAction = createAction("close", tr("Close"), tr("Close (Ctrl+W)"),
                                       "Ctrl+W", SLOT(closeFile()), ":/icons/toolbar/close.svg");
    m_toolbar->addAction(closeAction);

    QAction *closeAllAction = createAction("closeAll", tr("Close All"), tr("Close All Files"),
                                          "", SLOT(closeAllFiles()), ":/icons/toolbar/close-all.svg");
    m_toolbar->addAction(closeAllAction);

    m_toolbar->addSeparator();

    // Group 2 — Edit
    QAction *cutAction = createAction("cut", tr("Cut"), tr("Cut (Ctrl+X)"),
                                     "Ctrl+X", SLOT(cut()), ":/icons/toolbar/content-cut.svg");
    m_toolbar->addAction(cutAction);

    QAction *copyAction = createAction("copy", tr("Copy"), tr("Copy (Ctrl+C)"),
                                      "Ctrl+C", SLOT(copy()), ":/icons/toolbar/content-copy.svg");
    m_toolbar->addAction(copyAction);

    QAction *pasteAction = createAction("paste", tr("Paste"), tr("Paste (Ctrl+V)"),
                                       "Ctrl+V", SLOT(paste()), ":/icons/toolbar/content-paste.svg");
    m_toolbar->addAction(pasteAction);

    QAction *undoAction = createAction("undo", tr("Undo"), tr("Undo (Ctrl+Z)"),
                                      "Ctrl+Z", SLOT(undo()), ":/icons/toolbar/undo.svg");
    m_toolbar->addAction(undoAction);

    QAction *redoAction = createAction("redo", tr("Redo"), tr("Redo (Ctrl+Y)"),
                                      "Ctrl+Y", SLOT(redo()), ":/icons/toolbar/redo.svg");
    m_toolbar->addAction(redoAction);

    m_toolbar->addSeparator();

    // Group 3 — Search
    QAction *findAction = createAction("find", tr("Find"), tr("Find (Ctrl+F)"),
                                      "Ctrl+F", SLOT(find()), ":/icons/toolbar/magnify.svg");
    m_toolbar->addAction(findAction);

    QAction *replaceAction = createAction("replace", tr("Replace"), tr("Replace (Ctrl+H)"),
                                         "Ctrl+H", SLOT(replace()), ":/icons/toolbar/find-replace.svg");
    m_toolbar->addAction(replaceAction);

    m_toolbar->addSeparator();

    // Group 4 — View
    QAction *zoomInAction = createAction("zoomIn", tr("Zoom In"), tr("Zoom In (Ctrl++)"),
                                        "Ctrl++", SLOT(zoomIn()), ":/icons/toolbar/magnify-plus.svg");
    m_toolbar->addAction(zoomInAction);

    QAction *zoomOutAction = createAction("zoomOut", tr("Zoom Out"), tr("Zoom Out (Ctrl+-)"),
                                         "Ctrl+-", SLOT(zoomOut()), ":/icons/toolbar/magnify-minus.svg");
    m_toolbar->addAction(zoomOutAction);

    QAction *wordWrapAction = createAction("wordWrap", tr("Wrap"), tr("Toggle Word Wrap"),
                                          "", SLOT(toggleWordWrap()), ":/icons/toolbar/wrap.svg");
    wordWrapAction->setCheckable(true);
    m_toolbar->addAction(wordWrapAction);

    QAction *whitespaceAction = createAction("showWhitespace", tr("Whitespace"),
                                            tr("Show Whitespace Characters"),
                                            "", SLOT(toggleWhitespace()), ":/icons/toolbar/keyboard-space.svg");
    whitespaceAction->setCheckable(true);
    m_toolbar->addAction(whitespaceAction);

    QAction *eolAction = createAction("showEOL", tr("EOL"), tr("Show End of Line"),
                                     "", SLOT(toggleEndOfLine()), ":/icons/toolbar/format-pilcrow.svg");
    eolAction->setCheckable(true);
    m_toolbar->addAction(eolAction);

    QAction *indentGuideAction = createAction("showIndent", tr("Indent Guide"),
                                             tr("Show Indent Guides"),
                                             "", SLOT(toggleIndentGuide()), ":/icons/toolbar/format-line-weight.svg");
    indentGuideAction->setCheckable(true);
    m_toolbar->addAction(indentGuideAction);

    m_toolbar->addSeparator();

    // Group 5 — Macro
    QAction *startRecAction = createAction("startRecording", tr("Record"),
                                          tr("Start Recording Macro (Ctrl+Shift+R)"),
                                          "", SLOT(startRecordingMacro()), ":/icons/toolbar/record-circle.svg");
    m_toolbar->addAction(startRecAction);

    QAction *stopRecAction = createAction("stopRecording", tr("Stop"),
                                         tr("Stop Recording Macro"),
                                         "", SLOT(stopRecordingMacro()), ":/icons/toolbar/stop-circle.svg");
    stopRecAction->setEnabled(false);
    m_toolbar->addAction(stopRecAction);

    QAction *playbackAction = createAction("playback", tr("Play"),
                                          tr("Play Macro (Ctrl+Shift+P)"),
                                          "", SLOT(playbackMacro()), ":/icons/toolbar/play-circle.svg");
    m_toolbar->addAction(playbackAction);

    m_toolbar->addSeparator();

    // Group 6 — Format
    QAction *commentAction = createAction("toggleComment", tr("Comment"),
                                         tr("Toggle Comment (Ctrl+/)"),
                                         "", SLOT(toggleComment()), ":/icons/toolbar/comment-outline.svg");
    m_toolbar->addAction(commentAction);

    QAction *indentAction = createAction("increaseIndent", tr("Indent"),
                                        tr("Increase Indent (Tab)"),
                                        "", SLOT(increaseIndent()), ":/icons/toolbar/format-indent-increase.svg");
    m_toolbar->addAction(indentAction);

    QAction *outdentAction = createAction("decreaseIndent", tr("Outdent"),
                                         tr("Decrease Indent (Shift+Tab)"),
                                         "", SLOT(decreaseIndent()), ":/icons/toolbar/format-indent-decrease.svg");
    m_toolbar->addAction(outdentAction);
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
        QIcon icon(iconName);
        if (!icon.isNull()) {
            action->setIcon(icon);
        } else {
            action->setIcon(m_mainWindow->style()->standardIcon(QStyle::SP_FileIcon));
        }
    }

    if (slot) {
        connect(action, SIGNAL(triggered()), m_mainWindow, slot);
    }

    m_actions[name] = action;
    return action;
}

void ToolbarManager::updateToolbarState()
{
    // Update action states based on current editor state
}

QAction* ToolbarManager::action(const QString &name) const
{
    return m_actions.value(name, nullptr);
}

#include "mainwindow.h"
#include "tabwidget.h"
#include "editor.h"
#include "statusbar.h"
#include "finddialog.h"
#include "preferencesdialog.h"
#include "commandpalette.h"
#include "terminalwidget.h"
#include "toolbarmanager.h"
#include "core/document.h"
#include "core/documentmanager.h"
#include "core/macrorecorder.h"
#include "utils/settings.h"
#include "utils/fileutils.h"
#include "syntax/languagemanager.h"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QAction>
#include <QActionGroup>
#include <QShortcut>
#include <QIcon>
#include <QStyle>
#include <QPrinter>
#include <QPrintDialog>
#include <QPixmap>
#include <QPainter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    setupMenus();
    setupToolBar();
    setupStatusBar();
    setupShortcuts();

    // Start recovery timer
    DocumentManager::instance().startRecoveryTimer();

    // Create initial empty document
    newFile();

    updateWindowTitle();
}

MainWindow::~MainWindow()
{
    DocumentManager::instance().stopRecoveryTimer();
}

void MainWindow::setupUi()
{
    setWindowTitle("Olive Notepad");
    setWindowIcon(QIcon(":/icons/olive_notepad_icon.png"));
    resize(1200, 800);
    setAcceptDrops(true);

    m_tabWidget = new TabWidget(this);
    setCentralWidget(m_tabWidget);

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);

    // Setup terminal dock widget
    m_terminal = new TerminalWidget(this);
    m_terminalDock = new QDockWidget(tr("Terminal"), this);
    m_terminalDock->setWidget(m_terminal);
    m_terminalDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_terminalDock);
    m_terminalDock->hide();  // Hidden by default
}

void MainWindow::setupMenus()
{
    // ============================================================
    // File menu
    // ============================================================
    m_fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *newAction = m_fileMenu->addAction(tr("&New"), this, &MainWindow::newFile);
    newAction->setShortcut(QKeySequence::New);

    QAction *openAction = m_fileMenu->addAction(tr("&Open..."), this, [this]() { openFile(); });
    openAction->setShortcut(QKeySequence::Open);

    m_recentFilesMenu = m_fileMenu->addMenu(tr("Open &Recent"));
    updateRecentFilesMenu();

    m_fileMenu->addSeparator();

    QAction *saveAction = m_fileMenu->addAction(tr("&Save"), this, &MainWindow::saveFile);
    saveAction->setShortcut(QKeySequence::Save);

    QAction *saveAsAction = m_fileMenu->addAction(tr("Save &As..."), this, &MainWindow::saveFileAs);
    saveAsAction->setShortcut(QKeySequence::SaveAs);

    QAction *saveAllAction = m_fileMenu->addAction(tr("Save A&ll"), this, &MainWindow::saveAllFiles);
    saveAllAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));

    m_fileMenu->addSeparator();

    QAction *closeAction = m_fileMenu->addAction(tr("&Close"), this, [this]() { closeFile(); });
    closeAction->setShortcut(QKeySequence::Close);

    QAction *closeAllAction = m_fileMenu->addAction(tr("Close All"), this, &MainWindow::closeAllFiles);

    m_fileMenu->addSeparator();

    QAction *printAction = m_fileMenu->addAction(tr("&Print..."), this, &MainWindow::printFile);
    printAction->setShortcut(QKeySequence::Print);

    m_fileMenu->addSeparator();

    QAction *exitAction = m_fileMenu->addAction(tr("E&xit"), this, &QWidget::close);
    exitAction->setShortcut(QKeySequence::Quit);

    // ============================================================
    // Edit menu
    // ============================================================
    m_editMenu = menuBar()->addMenu(tr("&Edit"));

    QAction *undoAction = m_editMenu->addAction(tr("&Undo"), this, &MainWindow::undo);
    undoAction->setShortcut(QKeySequence::Undo);

    QAction *redoAction = m_editMenu->addAction(tr("&Redo"), this, &MainWindow::redo);
    redoAction->setShortcut(QKeySequence::Redo);

    m_editMenu->addSeparator();

    QAction *cutAction = m_editMenu->addAction(tr("Cu&t"), this, &MainWindow::cut);
    cutAction->setShortcut(QKeySequence::Cut);

    QAction *copyAction = m_editMenu->addAction(tr("&Copy"), this, &MainWindow::copy);
    copyAction->setShortcut(QKeySequence::Copy);

    QAction *pasteAction = m_editMenu->addAction(tr("&Paste"), this, &MainWindow::paste);
    pasteAction->setShortcut(QKeySequence::Paste);

    m_editMenu->addSeparator();

    QAction *selectAllAction = m_editMenu->addAction(tr("Select &All"), this, &MainWindow::selectAll);
    selectAllAction->setShortcut(QKeySequence::SelectAll);

    QAction *columnEditorAction = m_editMenu->addAction(tr("Column Editor..."), this, &MainWindow::columnEditor);
    columnEditorAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_C));

    m_editMenu->addSeparator();

    // Line operations submenu
    QMenu *lineOpsMenu = m_editMenu->addMenu(tr("&Line Operations"));

    QAction *duplicateLineAction = lineOpsMenu->addAction(tr("&Duplicate Line"), this, &MainWindow::duplicateLine);
    duplicateLineAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));

    QAction *deleteLineAction = lineOpsMenu->addAction(tr("De&lete Line"), this, &MainWindow::deleteLine);
    deleteLineAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_K));

    QAction *moveLineUpAction = lineOpsMenu->addAction(tr("Move Line &Up"), this, &MainWindow::moveLineUp);
    moveLineUpAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Up));

    QAction *moveLineDownAction = lineOpsMenu->addAction(tr("Move Line &Down"), this, &MainWindow::moveLineDown);
    moveLineDownAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Down));

    lineOpsMenu->addSeparator();

    lineOpsMenu->addAction(tr("&Sort Lines (Ascending)"), this, &MainWindow::sortLinesAscending);
    lineOpsMenu->addAction(tr("Sort Lines (Descending)"), this, &MainWindow::sortLinesDescending);
    lineOpsMenu->addAction(tr("&Remove Duplicate Lines"), this, &MainWindow::removeDuplicateLines);
    lineOpsMenu->addAction(tr("&Join Lines"), this, &MainWindow::joinLines);
    lineOpsMenu->addAction(tr("&Trim Trailing Whitespace"), this, &MainWindow::trimTrailingWhitespace);

    // Case conversion submenu
    QMenu *caseMenu = m_editMenu->addMenu(tr("&Convert Case"));
    caseMenu->addAction(tr("&UPPERCASE"), this, &MainWindow::toUpperCase);
    caseMenu->addAction(tr("&lowercase"), this, &MainWindow::toLowerCase);
    caseMenu->addAction(tr("&Title Case"), this, &MainWindow::toTitleCase);

    m_editMenu->addSeparator();

    QAction *toggleCommentAction = m_editMenu->addAction(tr("Toggle &Comment"), this, &MainWindow::toggleComment);
    toggleCommentAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash));

    m_editMenu->addSeparator();

    QAction *preferencesAction = m_editMenu->addAction(tr("&Preferences..."), this, &MainWindow::showPreferences);
    preferencesAction->setShortcut(QKeySequence::Preferences);

    // ============================================================
    // Search menu
    // ============================================================
    m_searchMenu = menuBar()->addMenu(tr("&Search"));

    QAction *findAction = m_searchMenu->addAction(tr("&Find..."), this, &MainWindow::find);
    findAction->setShortcut(QKeySequence::Find);

    QAction *findNextAction = m_searchMenu->addAction(tr("Find &Next"), this, &MainWindow::findNext);
    findNextAction->setShortcut(QKeySequence::FindNext);

    QAction *findPrevAction = m_searchMenu->addAction(tr("Find &Previous"), this, &MainWindow::findPrevious);
    findPrevAction->setShortcut(QKeySequence::FindPrevious);

    QAction *replaceAction = m_searchMenu->addAction(tr("&Replace..."), this, &MainWindow::replace);
    replaceAction->setShortcut(QKeySequence::Replace);

    QAction *findInFilesAction = m_searchMenu->addAction(tr("Find in &Files..."));
    findInFilesAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F));
    connect(findInFilesAction, &QAction::triggered, this, [this]() {
        QMessageBox::information(this, tr("Find in Files"), tr("Search dialog coming in Task 13"));
    });

    QAction *incrementalSearchAction = m_searchMenu->addAction(tr("&Incremental Search"));
    incrementalSearchAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    connect(incrementalSearchAction, &QAction::triggered, this, [this]() {
        QMessageBox::information(this, tr("Incremental Search"), tr("Incremental search coming in Task 13"));
    });

    m_searchMenu->addSeparator();

    QAction *goToLineAction = m_searchMenu->addAction(tr("&Go to Line..."), this, &MainWindow::goToLineDialog);
    goToLineAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));

    // ============================================================
    // View menu
    // ============================================================
    m_viewMenu = menuBar()->addMenu(tr("&View"));

    QAction *wordWrapAction = m_viewMenu->addAction(tr("&Word Wrap"));
    wordWrapAction->setCheckable(true);
    wordWrapAction->setChecked(Settings::instance().wordWrap());
    connect(wordWrapAction, &QAction::toggled, this, &MainWindow::toggleWordWrap);

    QAction *lineNumbersAction = m_viewMenu->addAction(tr("&Line Numbers"));
    lineNumbersAction->setCheckable(true);
    lineNumbersAction->setChecked(Settings::instance().showLineNumbers());
    connect(lineNumbersAction, &QAction::toggled, this, &MainWindow::toggleLineNumbers);

    QAction *terminalAction = m_viewMenu->addAction(tr("&Terminal"));
    terminalAction->setCheckable(true);
    terminalAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Apostrophe));
    connect(terminalAction, &QAction::toggled, this, &MainWindow::toggleTerminal);

    m_viewMenu->addSeparator();

    QAction *zoomInAction = m_viewMenu->addAction(tr("Zoom &In"), this, &MainWindow::zoomIn);
    zoomInAction->setShortcut(QKeySequence::ZoomIn);

    QAction *zoomOutAction = m_viewMenu->addAction(tr("Zoom &Out"), this, &MainWindow::zoomOut);
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);

    QAction *resetZoomAction = m_viewMenu->addAction(tr("&Reset Zoom"), this, &MainWindow::resetZoom);
    resetZoomAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));

    m_viewMenu->addSeparator();

    QAction *alwaysOnTopAction = m_viewMenu->addAction(tr("Always on &Top"));
    alwaysOnTopAction->setCheckable(true);
    connect(alwaysOnTopAction, &QAction::toggled, this, &MainWindow::toggleAlwaysOnTop);

    QAction *showWhitespaceAction = m_viewMenu->addAction(tr("Show &Whitespace"));
    showWhitespaceAction->setCheckable(true);
    connect(showWhitespaceAction, &QAction::toggled, this, &MainWindow::toggleWhitespace);

    QAction *showEolAction = m_viewMenu->addAction(tr("Show &End of Line"));
    showEolAction->setCheckable(true);
    connect(showEolAction, &QAction::toggled, this, &MainWindow::toggleEndOfLine);

    QAction *showIndentGuideAction = m_viewMenu->addAction(tr("Show Indent &Guide"));
    showIndentGuideAction->setCheckable(true);
    connect(showIndentGuideAction, &QAction::toggled, this, &MainWindow::toggleIndentGuide);

    m_viewMenu->addSeparator();

    m_viewMenu->addAction(tr("Fold All"), this, &MainWindow::foldAll);
    m_viewMenu->addAction(tr("Unfold All"), this, &MainWindow::unfoldAll);

    m_viewMenu->addSeparator();

    m_viewMenu->addAction(tr("Summary..."), this, &MainWindow::showSummary);

    m_viewMenu->addSeparator();

    QAction *fullScreenAction = m_viewMenu->addAction(tr("&Full Screen"), this, &MainWindow::toggleFullScreen);
    fullScreenAction->setShortcut(QKeySequence::FullScreen);

    // ============================================================
    // Encoding menu
    // ============================================================
    m_encodingMenu = menuBar()->addMenu(tr("&Encoding"));
    QActionGroup *encGroup = new QActionGroup(this);

    QAction *encUtf8 = m_encodingMenu->addAction(tr("Encode in UTF-8"), this, &MainWindow::convertToUTF8);
    encUtf8->setCheckable(true);
    encUtf8->setChecked(true);
    encGroup->addAction(encUtf8);

    QAction *encUtf8Bom = m_encodingMenu->addAction(tr("Encode in UTF-8-BOM"), this, &MainWindow::convertToUTF8BOM);
    encUtf8Bom->setCheckable(true);
    encGroup->addAction(encUtf8Bom);

    QAction *encAnsi = m_encodingMenu->addAction(tr("Encode in ANSI"), this, &MainWindow::convertToANSI);
    encAnsi->setCheckable(true);
    encGroup->addAction(encAnsi);

    QAction *encUcs2Be = m_encodingMenu->addAction(tr("Encode in UCS-2 BE BOM"), this, &MainWindow::convertToUCS2BE);
    encUcs2Be->setCheckable(true);
    encGroup->addAction(encUcs2Be);

    QAction *encUcs2Le = m_encodingMenu->addAction(tr("Encode in UCS-2 LE BOM"), this, &MainWindow::convertToUCS2LE);
    encUcs2Le->setCheckable(true);
    encGroup->addAction(encUcs2Le);

    // ============================================================
    // Language menu
    // ============================================================
    m_languageMenu = menuBar()->addMenu(tr("&Language"));
    m_languageMenu->addAction(tr("Auto-detect"));
    m_languageMenu->addSeparator();

    QStringList langs = LanguageManager::instance().availableLanguages();
    std::sort(langs.begin(), langs.end());
    for (const QString &lang : langs) {
        QAction *a = m_languageMenu->addAction(lang);
        connect(a, &QAction::triggered, this, [this, lang]() {
            Editor *e = currentEditor();
            if (e) e->setLanguage(lang);
        });
    }

    // ============================================================
    // Settings menu
    // ============================================================
    m_settingsMenu = menuBar()->addMenu(tr("&Settings"));
    m_settingsMenu->addAction(tr("&Preferences..."), this, &MainWindow::showPreferences);
    m_settingsMenu->addAction(tr("&Style Configurator..."));
    m_settingsMenu->addAction(tr("&Shortcut Mapper..."));

    // ============================================================
    // Tools menu (Command Palette only)
    // ============================================================
    m_toolsMenu = menuBar()->addMenu(tr("&Tools"));

    QAction *commandPaletteAction = m_toolsMenu->addAction(tr("Command &Palette..."), this, &MainWindow::showCommandPalette);
    commandPaletteAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));

    // ============================================================
    // Macro menu
    // ============================================================
    m_macroMenu = menuBar()->addMenu(tr("&Macro"));
    m_macroMenu->addAction(tr("Start &Recording"), this, &MainWindow::startRecordingMacro, QKeySequence(Qt::Key_F9));
    m_macroMenu->addAction(tr("&Stop Recording"), this, &MainWindow::stopRecordingMacro, QKeySequence(Qt::SHIFT | Qt::Key_F9));
    m_macroMenu->addAction(tr("&Playback"), this, &MainWindow::playbackMacro, QKeySequence(Qt::Key_F10));
    m_macroMenu->addSeparator();
    m_macroMenu->addAction(tr("Save Macro..."), this, &MainWindow::saveMacro);
    m_macroMenu->addAction(tr("Load Macro..."), this, &MainWindow::loadMacro);

    // ============================================================
    // Run menu
    // ============================================================
    m_runMenu = menuBar()->addMenu(tr("&Run"));
    m_runMenu->addAction(tr("&Launch in Terminal"), this, &MainWindow::launchInTerminal);
    m_runMenu->addAction(tr("Open Containing &Folder"), this, &MainWindow::openContainingFolder);

    // ============================================================
    // Window menu (dynamically populated)
    // ============================================================
    m_windowMenu = menuBar()->addMenu(tr("&Window"));
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::updateWindowMenu);
    updateWindowMenu();

    // ============================================================
    // ? (Help) menu
    // ============================================================
    m_helpMenu = menuBar()->addMenu(tr("?"));

    m_helpMenu->addAction(tr("&About Olive Notepad"), this, &MainWindow::showAbout);
    m_helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
}

void MainWindow::setupToolBar()
{
    ToolbarManager *toolbarMgr = new ToolbarManager(this, this);
    toolbarMgr->createToolbars();
    m_mainToolBar = toolbarMgr->toolbar();
}

void MainWindow::setupStatusBar()
{
    m_statusBar = new StatusBarWidget(this);
    setStatusBar(m_statusBar);
}

void MainWindow::setupShortcuts()
{
    // Additional shortcuts not covered by menu actions
}

void MainWindow::newFile()
{
    Document *doc = DocumentManager::instance().createDocument();
    Editor *editor = createEditor(doc);

    QString title = tr("Untitled %1").arg(++m_untitledCounter);
    int index = m_tabWidget->addTab(editor, title);
    m_tabWidget->setTabIcon(index, TabBar::iconForFile(QString()));
    m_tabWidget->setCurrentIndex(index);

    updateWindowTitle();
}

void MainWindow::openFile(const QString &filePath)
{
    QString path = filePath;
    if (path.isEmpty()) {
        path = QFileDialog::getOpenFileName(this, tr("Open File"), QString(),
                                            FileUtils::combinedFilter());
    }

    if (path.isEmpty()) {
        return;
    }

    // Check if already open
    int existingIndex = -1;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        Editor *ed = qobject_cast<Editor*>(m_tabWidget->widget(i));
        if (ed && ed->document() && ed->document()->filePath() == path) {
            existingIndex = i;
            break;
        }
    }

    if (existingIndex >= 0) {
        m_tabWidget->setCurrentIndex(existingIndex);
        return;
    }

    Document *doc = DocumentManager::instance().openDocument(path);
    if (!doc) {
        QMessageBox::warning(this, tr("Error"), tr("Could not open file: %1").arg(path));
        return;
    }

    // Detect language
    QString ext = FileUtils::fileExtension(path);
    QString language = LanguageManager::instance().languageForExtension(ext);
    doc->setLanguage(language);

    Editor *editor = createEditor(doc);
    int index = m_tabWidget->addTab(editor, doc->displayName());
    m_tabWidget->setTabIcon(index, TabBar::iconForFile(path));
    m_tabWidget->setCurrentIndex(index);

    updateWindowTitle();
}

void MainWindow::saveFile()
{
    Editor *editor = currentEditor();
    if (!editor || !editor->document()) {
        return;
    }

    // Sync editor content to document before saving
    editor->syncToDocument();

    Document *doc = editor->document();
    if (doc->isUntitled()) {
        saveFileAs();
        return;
    }

    if (doc->save()) {
        updateWindowTitle();
        int index = findEditorIndex(editor);
        if (index >= 0) {
            m_tabWidget->setTabText(index, doc->displayName());
        }
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Could not save file."));
    }
}

void MainWindow::saveFileAs()
{
    Editor *editor = currentEditor();
    if (!editor || !editor->document()) {
        return;
    }

    QString path = QFileDialog::getSaveFileName(this, tr("Save File As"), QString(),
                                                 FileUtils::combinedFilter());
    if (path.isEmpty()) {
        return;
    }

    // Sync editor content to document before saving
    editor->syncToDocument();

    Document *doc = editor->document();
    if (doc->saveAs(path)) {
        // Update language based on new extension
        QString ext = FileUtils::fileExtension(path);
        QString language = LanguageManager::instance().languageForExtension(ext);
        doc->setLanguage(language);

        updateWindowTitle();
        int index = findEditorIndex(editor);
        if (index >= 0) {
            m_tabWidget->setTabText(index, doc->displayName());
        }
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Could not save file."));
    }
}

void MainWindow::saveAllFiles()
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        Editor *editor = qobject_cast<Editor*>(m_tabWidget->widget(i));
        if (editor && editor->document() && editor->document()->isModified()) {
            m_tabWidget->setCurrentIndex(i);
            saveFile();
        }
    }
}

bool MainWindow::closeFile(int index)
{
    if (index < 0) {
        index = m_tabWidget->currentIndex();
    }

    if (index < 0 || index >= m_tabWidget->count()) {
        return false;
    }

    Editor *editor = qobject_cast<Editor*>(m_tabWidget->widget(index));
    if (!editor) {
        return false;
    }

    Document *doc = editor->document();
    if (doc && doc->isModified()) {
        QMessageBox::StandardButton result = QMessageBox::question(
            this, tr("Save Changes"),
            tr("Do you want to save changes to '%1'?").arg(doc->displayName()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (result == QMessageBox::Cancel) {
            return false;
        }
        if (result == QMessageBox::Save) {
            m_tabWidget->setCurrentIndex(index);
            saveFile();
            if (doc->isModified()) {
                return false;  // Save failed or canceled
            }
        }
    }

    m_tabWidget->removeTab(index);
    if (doc) {
        DocumentManager::instance().closeDocument(doc, true);
    }

    // Create new empty tab if all closed (but not if we're closing all to exit)
    if (m_tabWidget->count() == 0 && !m_closingAll) {
        newFile();
    }

    updateWindowTitle();
    return true;
}

bool MainWindow::closeAllFiles()
{
    // Prevent closeFile from creating new tabs while we're closing all
    m_closingAll = true;
    while (m_tabWidget->count() > 0) {
        if (!closeFile(0)) {
            m_closingAll = false;
            return false;
        }
    }
    m_closingAll = false;
    return true;
}

void MainWindow::goToLine(int line)
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->goToLine(line);
    }
}

int MainWindow::currentTabIndex() const
{
    return m_tabWidget->currentIndex();
}

void MainWindow::setCurrentTabIndex(int index)
{
    if (index >= 0 && index < m_tabWidget->count()) {
        m_tabWidget->setCurrentIndex(index);
    }
}

Editor *MainWindow::currentEditor() const
{
    return qobject_cast<Editor*>(m_tabWidget->currentWidget());
}

QStringList MainWindow::openFilePaths() const
{
    QStringList paths;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        Editor *editor = qobject_cast<Editor*>(m_tabWidget->widget(i));
        if (editor && editor->document() && !editor->document()->isUntitled()) {
            paths.append(editor->document()->filePath());
        }
    }
    return paths;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (closeAllFiles()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        for (const QUrl &url : mimeData->urls()) {
            if (url.isLocalFile()) {
                openFile(url.toLocalFile());
            }
        }
    }
}

void MainWindow::onTabChanged(int index)
{
    updateWindowTitle();
    updateStatusBar();
    updateMenuState();
}

void MainWindow::onTabCloseRequested(int index)
{
    closeFile(index);
}

void MainWindow::onDocumentModified(bool modified)
{
    Editor *editor = qobject_cast<Editor*>(sender());
    if (!editor) {
        Document *doc = qobject_cast<Document*>(sender());
        if (doc) {
            int index = findEditorIndex(doc);
            if (index >= 0) {
                QString title = doc->displayName();
                if (modified) {
                    title += " *";
                }
                m_tabWidget->setTabText(index, title);

                // Update icon: tint red when modified, restore when saved
                QString filePath = doc->filePath();
                if (modified) {
                    QPixmap px(16, 16);
                    px.fill(QColor("#e53935"));
                    QPainter p(&px);
                    QFont f = p.font();
                    f.setPixelSize(7);
                    f.setBold(true);
                    p.setFont(f);
                    p.setPen(Qt::white);
                    p.drawText(px.rect(), Qt::AlignCenter, "MOD");
                    p.end();
                    m_tabWidget->setTabIcon(index, QIcon(px));
                } else {
                    m_tabWidget->setTabIcon(index,
                        TabBar::iconForFile(filePath));
                }
            }
        }
        return;
    }

    int index = findEditorIndex(editor);
    if (index >= 0 && editor->document()) {
        QString title = editor->document()->displayName();
        if (modified) {
            title += " *";
        }
        m_tabWidget->setTabText(index, title);

        // Update icon: tint red when modified, restore when saved
        QString filePath = editor->document()->filePath();
        if (modified) {
            QPixmap px(16, 16);
            px.fill(QColor("#e53935"));
            QPainter p(&px);
            QFont f = p.font();
            f.setPixelSize(7);
            f.setBold(true);
            p.setFont(f);
            p.setPen(Qt::white);
            p.drawText(px.rect(), Qt::AlignCenter, "MOD");
            p.end();
            m_tabWidget->setTabIcon(index, QIcon(px));
        } else {
            m_tabWidget->setTabIcon(index,
                TabBar::iconForFile(filePath));
        }
    }

    updateWindowTitle();
}

void MainWindow::onCursorPositionChanged()
{
    updateStatusBar();
}

void MainWindow::undo()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->undo();
    }
}

void MainWindow::redo()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->redo();
    }
}

void MainWindow::cut()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->cut();
    }
}

void MainWindow::copy()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->copy();
    }
}

void MainWindow::paste()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->paste();
    }
}

void MainWindow::selectAll()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->selectAll();
    }
}

void MainWindow::printFile()
{
    Editor *editor = currentEditor();
    if (editor) {
        QPrinter printer(QPrinter::HighResolution);
        QPrintDialog dialog(&printer, this);
        dialog.setWindowTitle(tr("Print Document"));

        if (dialog.exec() == QDialog::Accepted) {
            editor->print(&printer);
        }
    }
}

void MainWindow::increaseIndent()
{
    Editor *editor = currentEditor();
    if (editor) {
        QTextCursor cursor = editor->textCursor();
        cursor.beginEditBlock();

        if (cursor.hasSelection()) {
            // Indent all selected lines
            int start = cursor.selectionStart();
            int end = cursor.selectionEnd();

            cursor.setPosition(start);
            cursor.movePosition(QTextCursor::StartOfLine);

            while (cursor.position() <= end && !cursor.atEnd()) {
                cursor.insertText("\t");
                if (!cursor.movePosition(QTextCursor::Down)) {
                    break;
                }
                cursor.movePosition(QTextCursor::StartOfLine);
                end++; // Account for inserted tab
            }
        } else {
            // Indent current line
            cursor.movePosition(QTextCursor::StartOfLine);
            cursor.insertText("\t");
        }

        cursor.endEditBlock();
    }
}

void MainWindow::decreaseIndent()
{
    Editor *editor = currentEditor();
    if (editor) {
        QTextCursor cursor = editor->textCursor();
        cursor.beginEditBlock();

        if (cursor.hasSelection()) {
            // Unindent all selected lines
            int start = cursor.selectionStart();
            int end = cursor.selectionEnd();

            cursor.setPosition(start);
            cursor.movePosition(QTextCursor::StartOfLine);

            while (cursor.position() <= end && !cursor.atEnd()) {
                // Remove one tab or up to 4 spaces from start of line
                cursor.movePosition(QTextCursor::StartOfLine);
                cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
                QString lineText = cursor.selectedText();

                cursor.movePosition(QTextCursor::StartOfLine);
                if (lineText.startsWith('\t')) {
                    cursor.deleteChar();
                    end--;
                } else if (lineText.startsWith("    ")) {
                    for (int i = 0; i < 4; i++) {
                        cursor.deleteChar();
                        end--;
                    }
                }

                if (!cursor.movePosition(QTextCursor::Down)) {
                    break;
                }
            }
        } else {
            // Unindent current line
            cursor.movePosition(QTextCursor::StartOfLine);
            cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            QString lineText = cursor.selectedText();

            cursor.movePosition(QTextCursor::StartOfLine);
            if (lineText.startsWith('\t')) {
                cursor.deleteChar();
            } else if (lineText.startsWith("    ")) {
                for (int i = 0; i < 4; i++) {
                    cursor.deleteChar();
                }
            }
        }

        cursor.endEditBlock();
    }
}

void MainWindow::duplicateLine()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->duplicateLine();
    }
}

void MainWindow::deleteLine()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->deleteLine();
    }
}

void MainWindow::moveLineUp()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->moveLineUp();
    }
}

void MainWindow::moveLineDown()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->moveLineDown();
    }
}

void MainWindow::toggleComment()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->toggleComment();
    }
}

void MainWindow::sortLinesAscending()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->sortLinesAscending();
    }
}

void MainWindow::sortLinesDescending()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->sortLinesDescending();
    }
}

void MainWindow::removeDuplicateLines()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->removeDuplicateLines();
    }
}

void MainWindow::joinLines()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->joinLines();
    }
}

void MainWindow::trimTrailingWhitespace()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->trimTrailingWhitespace();
    }
}

void MainWindow::toUpperCase()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->toUpperCase();
    }
}

void MainWindow::toLowerCase()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->toLowerCase();
    }
}

void MainWindow::toTitleCase()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->toTitleCase();
    }
}

void MainWindow::find()
{
    if (!m_findDialog) {
        m_findDialog = new FindDialog(this);
        connect(m_findDialog, &FindDialog::findNext, this, &MainWindow::findNext);
        connect(m_findDialog, &FindDialog::findPrevious, this, &MainWindow::findPrevious);
    }
    m_findDialog->setReplaceMode(false);
    m_findDialog->show();
    m_findDialog->raise();
    m_findDialog->activateWindow();
}

void MainWindow::findNext()
{
    Editor *editor = currentEditor();
    if (editor && m_findDialog) {
        editor->findNext(m_findDialog->searchText(), m_findDialog->searchFlags());
    }
}

void MainWindow::findPrevious()
{
    Editor *editor = currentEditor();
    if (editor && m_findDialog) {
        editor->findPrevious(m_findDialog->searchText(), m_findDialog->searchFlags());
    }
}

void MainWindow::replace()
{
    if (!m_findDialog) {
        m_findDialog = new FindDialog(this);
    }
    m_findDialog->setReplaceMode(true);
    m_findDialog->show();
    m_findDialog->raise();
    m_findDialog->activateWindow();
}

void MainWindow::goToLineDialog()
{
    Editor *editor = currentEditor();
    if (!editor) {
        return;
    }

    bool ok;
    int line = QInputDialog::getInt(this, tr("Go to Line"), tr("Line number:"),
                                    1, 1, INT_MAX, 1, &ok);
    if (ok) {
        goToLine(line);
    }
}

void MainWindow::toggleWordWrap()
{
    bool wrap = !Settings::instance().wordWrap();
    Settings::instance().setWordWrap(wrap);

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        Editor *editor = qobject_cast<Editor*>(m_tabWidget->widget(i));
        if (editor) {
            editor->setWordWrapEnabled(wrap);
        }
    }
}

void MainWindow::toggleLineNumbers()
{
    bool show = !Settings::instance().showLineNumbers();
    Settings::instance().setShowLineNumbers(show);

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        Editor *editor = qobject_cast<Editor*>(m_tabWidget->widget(i));
        if (editor) {
            editor->setLineNumbersVisible(show);
        }
    }
}

void MainWindow::toggleTerminal()
{
    if (m_terminalDock) {
        m_terminalDock->setVisible(!m_terminalDock->isVisible());
    }
}

void MainWindow::zoomIn()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->zoomIn();
    }
}

void MainWindow::zoomOut()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->zoomOut();
    }
}

void MainWindow::resetZoom()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->resetZoom();
    }
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("About Olive Notepad"),
        tr("<h2>Olive Notepad 1.0</h2>"
           "<p>A native Linux text editor designed to match and exceed "
           "Notepad++ functionality while incorporating modern features.</p>"
           "<p>Built with Qt %1</p>").arg(QT_VERSION_STR));
}

void MainWindow::showPreferences()
{
    if (!m_preferencesDialog) {
        m_preferencesDialog = new PreferencesDialog(this);
    }
    m_preferencesDialog->show();
    m_preferencesDialog->raise();
    m_preferencesDialog->activateWindow();
}

void MainWindow::showCommandPalette()
{
    if (!m_commandPalette) {
        m_commandPalette = new CommandPalette(this);
        populateCommandPalette();
    }
    m_commandPalette->showAndFocus();
}

void MainWindow::populateCommandPalette()
{
    if (!m_commandPalette) return;

    m_commandPalette->clearActions();

    // Add all actions from menus
    // File menu
    for (QAction *action : m_fileMenu->actions()) {
        if (!action->isSeparator() && !action->text().isEmpty()) {
            m_commandPalette->addAction(action, tr("File"));
        }
    }

    // Edit menu
    for (QAction *action : m_editMenu->actions()) {
        if (!action->isSeparator() && !action->text().isEmpty()) {
            if (action->menu()) {
                // Submenu - add its actions
                for (QAction *subAction : action->menu()->actions()) {
                    if (!subAction->isSeparator() && !subAction->text().isEmpty()) {
                        m_commandPalette->addAction(subAction, tr("Edit"));
                    }
                }
            } else {
                m_commandPalette->addAction(action, tr("Edit"));
            }
        }
    }

    // Search menu
    for (QAction *action : m_searchMenu->actions()) {
        if (!action->isSeparator() && !action->text().isEmpty()) {
            m_commandPalette->addAction(action, tr("Search"));
        }
    }

    // View menu
    for (QAction *action : m_viewMenu->actions()) {
        if (!action->isSeparator() && !action->text().isEmpty()) {
            m_commandPalette->addAction(action, tr("View"));
        }
    }

    // Tools menu
    for (QAction *action : m_toolsMenu->actions()) {
        if (!action->isSeparator() && !action->text().isEmpty()) {
            m_commandPalette->addAction(action, tr("Tools"));
        }
    }

    // Help menu
    for (QAction *action : m_helpMenu->actions()) {
        if (!action->isSeparator() && !action->text().isEmpty()) {
            m_commandPalette->addAction(action, tr("Help"));
        }
    }
}

void MainWindow::startRecordingMacro()
{
    MacroRecorder::instance().startRecording();
    statusBar()->showMessage(tr("Recording macro... (F9 to stop)"), 0);
}

void MainWindow::stopRecordingMacro()
{
    MacroRecorder::instance().stopRecording();
    QString desc = MacroRecorder::instance().currentMacroDescription();
    statusBar()->showMessage(tr("Macro recording stopped: %1").arg(desc), 5000);
}

void MainWindow::playbackMacro()
{
    Editor *editor = currentEditor();
    if (!editor) {
        QMessageBox::warning(this, tr("Playback Macro"),
                             tr("No active editor to playback macro."));
        return;
    }

    if (MacroRecorder::instance().isRecording()) {
        QMessageBox::warning(this, tr("Playback Macro"),
                             tr("Cannot playback while recording. Stop recording first."));
        return;
    }

    MacroRecorder::instance().playback(editor);
    statusBar()->showMessage(tr("Macro playback complete"), 3000);
}

void MainWindow::saveMacro()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Save Macro"),
                                          tr("Macro name:"), QLineEdit::Normal,
                                          "", &ok);
    if (ok && !name.isEmpty()) {
        MacroRecorder::instance().saveMacro(name);
        statusBar()->showMessage(tr("Macro saved: %1").arg(name), 3000);
    }
}

void MainWindow::loadMacro()
{
    QStringList macros = MacroRecorder::instance().savedMacroNames();
    if (macros.isEmpty()) {
        QMessageBox::information(this, tr("Load Macro"),
                                  tr("No saved macros found."));
        return;
    }

    bool ok;
    QString name = QInputDialog::getItem(this, tr("Load Macro"),
                                          tr("Select macro:"), macros,
                                          0, false, &ok);
    if (ok && !name.isEmpty()) {
        MacroRecorder::instance().loadMacro(name);
        QString desc = MacroRecorder::instance().currentMacroDescription();
        statusBar()->showMessage(tr("Macro loaded: %1 - %2").arg(name, desc), 5000);
    }
}

void MainWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (action) {
        openFile(action->data().toString());
    }
}

void MainWindow::clearRecentFiles()
{
    DocumentManager::instance().clearRecentFiles();
    updateRecentFilesMenu();
}

void MainWindow::updateRecentFilesMenu()
{
    m_recentFilesMenu->clear();

    const QStringList &recentFiles = DocumentManager::instance().recentFiles();
    for (const QString &file : recentFiles) {
        QAction *action = m_recentFilesMenu->addAction(FileUtils::fileName(file));
        action->setData(file);
        action->setToolTip(file);
        connect(action, &QAction::triggered, this, &MainWindow::openRecentFile);
    }

    if (!recentFiles.isEmpty()) {
        m_recentFilesMenu->addSeparator();
        m_recentFilesMenu->addAction(tr("Clear Recent Files"), this, &MainWindow::clearRecentFiles);
    }

    m_recentFilesMenu->setEnabled(!recentFiles.isEmpty());
}

void MainWindow::updateWindowTitle()
{
    Editor *editor = currentEditor();
    if (editor && editor->document()) {
        Document *doc = editor->document();
        QString title = doc->displayName();
        if (doc->isModified()) {
            title += " *";
        }
        if (!doc->isUntitled()) {
            title += " - " + doc->filePath();
        }
        title += " - Olive Notepad";
        setWindowTitle(title);
    } else {
        setWindowTitle("Olive Notepad");
    }
}

void MainWindow::updateStatusBar()
{
    Editor *editor = currentEditor();
    if (editor) {
        m_statusBar->updateFromEditor(editor);
    }
}

void MainWindow::updateMenuState()
{
    // Enable/disable menu items based on current state
}

Editor *MainWindow::createEditor(Document *document)
{
    Editor *editor = new Editor(document, this);

    connect(document, &Document::modifiedChanged, this, &MainWindow::onDocumentModified);
    connect(editor, &Editor::cursorPositionChanged, this, &MainWindow::onCursorPositionChanged);

    return editor;
}

int MainWindow::findEditorIndex(Editor *editor) const
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabWidget->widget(i) == editor) {
            return i;
        }
    }
    return -1;
}

int MainWindow::findEditorIndex(Document *document) const
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        Editor *editor = qobject_cast<Editor*>(m_tabWidget->widget(i));
        if (editor && editor->document() == document) {
            return i;
        }
    }
    return -1;
}


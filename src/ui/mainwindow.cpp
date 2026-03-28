#include "mainwindow.h"
#include "tabwidget.h"
#include "editor.h"
#include "theme.h"
#include "lspbridge.h"
#include "core/lspmanager.h"
#include "core/lspclient.h"
#include "core/llmevaluator.h"
#include "statusbar.h"
#include "searchdialog.h"
#include "searchresultspanel.h"
#include "incrementalsearchbar.h"
#include "core/searchengine.h"
#include "preferencesdialog.h"
#include "commandpalette.h"
#include "terminalwidget.h"
#include "toolbarmanager.h"
#include "notificationbar.h"
#include "documentmap.h"
#include "functionlist.h"
#include "macrodialog.h"
#include "evalresultwidget.h"
#include "endpointconfigdialog.h"
#include "gitcommitdialog.h"
#include "plugindialog.h"
#include "core/document.h"
#include "core/documentmanager.h"
#include "core/session.h"
#include "core/macrorecorder.h"
#include "core/pluginmanager.h"
#include "core/remoteconnection.h"
#include "remoteconnectiondialog.h"
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
#include <QVBoxLayout>
#include <QTextDocument>
#include <QAction>
#include <QActionGroup>
#include <QShortcut>
#include <QIcon>
#include <QStyle>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPixmap>
#include <QPainter>
#include <QFileInfo>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QDir>
#include <QTextBlock>
#include <QLocale>
#include <QSettings>
#include <QClipboard>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    setupMenus();
    setupToolBar();
    setupStatusBar();
    setupShortcuts();

    // Clipboard tracking for Paste action
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, [this]() {
        if (m_pasteAction) m_pasteAction->setEnabled(!QApplication::clipboard()->text().isEmpty());
    });

    // Start recovery timer
    DocumentManager::instance().startRecoveryTimer();

    // Plugin system
    initPluginSystem();

    // Create initial empty document
    newFile();

    updateWindowTitle();
}

MainWindow::~MainWindow()
{
    PluginManager::instance().unloadAllPlugins();
    DocumentManager::instance().stopRecoveryTimer();
    delete m_editorAPI;
    m_editorAPI = nullptr;
}

void MainWindow::setupUi()
{
    setWindowTitle("PrimeEdit");
    setWindowIcon(QIcon(":/icons/prime_edit_icon.png"));
    resize(1200, 800);
    setAcceptDrops(true);

    m_tabWidget = new TabWidget(this);

    // Wrap tab widget with incremental search bar
    QWidget *centralContainer = new QWidget(this);
    QVBoxLayout *centralLayout = new QVBoxLayout(centralContainer);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    m_incrementalSearchBar = new IncrementalSearchBar(centralContainer);
    m_incrementalSearchBar->hide();
    m_notificationBar = new NotificationBar(centralContainer);
    centralLayout->addWidget(m_incrementalSearchBar);
    centralLayout->addWidget(m_notificationBar);

    // Use a QSplitter to hold tab widgets for split view
    m_splitter = new QSplitter(Qt::Horizontal, centralContainer);
    m_splitter->addWidget(m_tabWidget);
    centralLayout->addWidget(m_splitter);
    setCentralWidget(centralContainer);

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);

    // Track last-focused tab widget so activeTabWidget() works during menus/dialogs
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget*, QWidget *now) {
        if (!now) return;
        QWidget *w = now;
        while (w) {
            if (w == m_tabWidget) { m_lastActiveTabWidget = m_tabWidget; return; }
            if (w == m_tabWidget2) { m_lastActiveTabWidget = m_tabWidget2; return; }
            w = w->parentWidget();
        }
    });

    connect(m_incrementalSearchBar, &IncrementalSearchBar::searchChanged,
            this, &MainWindow::onIncrementalSearchChanged);
    connect(m_incrementalSearchBar, &IncrementalSearchBar::findNext, this, &MainWindow::findNext);
    connect(m_incrementalSearchBar, &IncrementalSearchBar::findPrevious, this, &MainWindow::findPrevious);

    // Setup search results dock widget
    m_searchResultsPanel = new SearchResultsPanel(this);
    m_searchResultsDock = new QDockWidget(tr("Search Results"), this);
    m_searchResultsDock->setWidget(m_searchResultsPanel);
    m_searchResultsDock->setAllowedAreas(Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_searchResultsDock);
    m_searchResultsDock->hide();
    connect(m_searchResultsPanel, &SearchResultsPanel::resultActivated,
            this, &MainWindow::onSearchResultActivated);

    // Setup terminal dock widget
    m_terminal = new TerminalWidget(this);
    m_terminalDock = new QDockWidget(tr("Terminal"), this);
    m_terminalDock->setWidget(m_terminal);
    m_terminalDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_terminalDock);
    m_terminalDock->hide();  // Hidden by default

    // Setup document map dock widget
    m_documentMap = new DocumentMapWidget(this);
    m_documentMapDock = new QDockWidget(tr("Document Map"), this);
    m_documentMapDock->setWidget(m_documentMap);
    m_documentMapDock->setAllowedAreas(Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, m_documentMapDock);
    m_documentMapDock->hide();

    // Setup function list dock widget
    m_functionList = new FunctionListPanel(this);
    m_functionListDock = new QDockWidget(tr("Function List"), this);
    m_functionListDock->setWidget(m_functionList);
    m_functionListDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, m_functionListDock);
    m_functionListDock->hide();

    connect(m_functionList, &FunctionListPanel::functionActivated, this, [this](int line) {
        Editor *e = currentEditor();
        if (e) e->goToLine(line);
    });

    // File change monitoring
    connect(&DocumentManager::instance(),
            &DocumentManager::fileExternallyModified,
            this, [this](const QString &path) {
        m_notificationBar->setProperty("reloadPath", path);
        m_notificationBar->showMessage(
            tr("The file \"%1\" has been modified by another "
               "program. Reload?")
                .arg(QFileInfo(path).fileName()));
    });
    connect(m_notificationBar, &NotificationBar::accepted,
            this, [this]() {
        QString path = m_notificationBar->property("reloadPath")
                           .toString();
        if (path.isEmpty()) return;
        Document *doc =
            DocumentManager::instance().findDocument(path);
        if (doc) {
            doc->load(path);
            // Sync editor if this doc is currently displayed
            Editor *e = currentEditor();
            if (e && e->document() == doc) {
                e->syncFromDocument();
            }
        }
    });

    // LSP Bridge
    m_lspBridge = new LSPBridge(this, this);

    // LLM Evaluator
    m_evaluator = new LLMEvaluator(this);
    connect(m_evaluator, &LLMEvaluator::resultReady,
            this, &MainWindow::onEvalResult);
    connect(m_evaluator, &LLMEvaluator::errorOccurred,
            this, [this](const QString &err) {
        if (m_evalResult) m_evalResult->hide();
        statusBar()->showMessage(tr("Evaluation error: %1").arg(err), 5000);
    });

    // Connect settings signals to apply changes globally
    connect(&Settings::instance(), &Settings::themeChanged, this, [this](const QString &themeName) {
        // applyTheme() already calls qApp->setStyleSheet() and emits themeChanged
        // Do NOT call setStyleSheet again — double application during signal processing crashes
        ThemeManager::instance().applyTheme(themeName);
    });

    // Periodic unsaved document backup (crash safety net)
    // Saves every 30 seconds so a crash never loses more than 30s of work
    QTimer *unsavedBackupTimer = new QTimer(this);
    unsavedBackupTimer->setInterval(30000);
    connect(unsavedBackupTimer, &QTimer::timeout, this, [this]() {
        Session::instance().saveUnsavedDocuments(this);
    });
    unsavedBackupTimer->start();

    connect(&Settings::instance(), &Settings::fontChanged, this, [this]() {
        // Re-apply theme to pick up font changes in stylesheets
        ThemeManager::instance().applyTheme(ThemeManager::instance().currentTheme());
    });

    connect(&Settings::instance(), &Settings::wordWrapChanged, this, [this](bool enabled) {
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            Editor *e = qobject_cast<Editor*>(m_tabWidget->widget(i));
            if (e) e->setWordWrapEnabled(enabled);
        }
    });

    connect(&Settings::instance(), &Settings::lineNumbersChanged, this, [this](bool enabled) {
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            Editor *e = qobject_cast<Editor*>(m_tabWidget->widget(i));
            if (e) e->setLineNumbersVisible(enabled);
        }
    });

    connect(&Settings::instance(), &Settings::highlightCurrentLineChanged, this, [this](bool) {
        // Trigger re-highlight on all open editors by moving cursor
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            Editor *e = qobject_cast<Editor*>(m_tabWidget->widget(i));
            if (e) {
                // Force rehighlight by toggling extra selections
                QTextCursor cursor = e->textCursor();
                e->setTextCursor(cursor);
            }
        }
    });

    connect(&Settings::instance(), &Settings::minimapChanged, this, [this](bool enabled) {
        if (m_documentMapDock) {
            m_documentMapDock->setVisible(enabled);
        }
    });

    connect(&Settings::instance(), &Settings::autoSaveChanged, this, [](bool enabled) {
        if (enabled) {
            DocumentManager::instance().startRecoveryTimer();
        } else {
            DocumentManager::instance().stopRecoveryTimer();
        }
    });
}

void MainWindow::setupMenus()
{
    // ============================================================
    // File menu
    // ============================================================
    m_fileMenu = menuBar()->addMenu(tr("&File"));

    m_newAction = m_fileMenu->addAction(tr("&New"), this, &MainWindow::newFile);
    m_newAction->setShortcut(QKeySequence::New);

    m_openAction = m_fileMenu->addAction(tr("&Open..."), this, [this]() { openFile(); });
    m_openAction->setShortcut(QKeySequence::Open);

    m_recentFilesMenu = m_fileMenu->addMenu(tr("Open &Recent"));
    updateRecentFilesMenu();

    QAction *openRemoteAction = m_fileMenu->addAction(
        tr("Open Re&mote..."), this, &MainWindow::openRemoteFile);
    openRemoteAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));

    m_fileMenu->addSeparator();

    m_saveAction = m_fileMenu->addAction(tr("&Save"), this, &MainWindow::saveFile);
    m_saveAction->setShortcut(QKeySequence::Save);

    QAction *saveAsAction = m_fileMenu->addAction(tr("Save &As..."), this, &MainWindow::saveFileAs);
    saveAsAction->setShortcut(QKeySequence::SaveAs);

    m_saveAllAction = m_fileMenu->addAction(tr("Save A&ll"), this, &MainWindow::saveAllFiles);
    m_saveAllAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));

    QAction *reloadAction = m_fileMenu->addAction(tr("Reload from Disk"), this, &MainWindow::reloadFromDisk);
    reloadAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));

    m_fileMenu->addSeparator();

    m_closeAction = m_fileMenu->addAction(tr("&Close"), this, [this]() { closeFile(); });
    m_closeAction->setShortcut(QKeySequence::Close);

    QAction *closeAllAction = m_fileMenu->addAction(tr("Close All"), this, &MainWindow::closeAllFiles);

    m_fileMenu->addSeparator();

    m_printAction = m_fileMenu->addAction(tr("&Print..."), this, &MainWindow::printFile);
    m_printAction->setShortcut(QKeySequence::Print);

    m_fileMenu->addSeparator();

    QAction *exitAction = m_fileMenu->addAction(tr("E&xit"), this, &QWidget::close);
    exitAction->setShortcut(QKeySequence::Quit);

    // ============================================================
    // Edit menu
    // ============================================================
    m_editMenu = menuBar()->addMenu(tr("&Edit"));

    m_undoAction = m_editMenu->addAction(tr("&Undo"), this, &MainWindow::undo);
    m_undoAction->setShortcut(QKeySequence::Undo);

    m_redoAction = m_editMenu->addAction(tr("&Redo"), this, &MainWindow::redo);
    m_redoAction->setShortcut(QKeySequence::Redo);

    m_editMenu->addSeparator();

    m_cutAction = m_editMenu->addAction(tr("Cu&t"), this, &MainWindow::cut);
    m_cutAction->setShortcut(QKeySequence::Cut);

    m_copyAction = m_editMenu->addAction(tr("&Copy"), this, &MainWindow::copy);
    m_copyAction->setShortcut(QKeySequence::Copy);

    m_pasteAction = m_editMenu->addAction(tr("&Paste"), this, &MainWindow::paste);
    m_pasteAction->setShortcut(QKeySequence::Paste);

    m_editMenu->addSeparator();

    QAction *selectAllAction = m_editMenu->addAction(tr("Select &All"), this, &MainWindow::selectAll);
    selectAllAction->setShortcut(QKeySequence::SelectAll);

    QAction *columnEditorAction = m_editMenu->addAction(tr("Column Editor..."), this, &MainWindow::columnEditor);
    columnEditorAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_C));

    m_editMenu->addSeparator();

    // Line operations submenu
    QMenu *lineOpsMenu = m_editMenu->addMenu(tr("&Line Operations"));

    m_duplicateLineAction = lineOpsMenu->addAction(tr("&Duplicate Line"), this, &MainWindow::duplicateLine);
    m_duplicateLineAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));

    m_deleteLineAction = lineOpsMenu->addAction(tr("De&lete Line"), this, &MainWindow::deleteLine);
    m_deleteLineAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_K));

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

    m_toggleCommentAction = m_editMenu->addAction(tr("Toggle &Comment"), this, &MainWindow::toggleComment);
    m_toggleCommentAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash));

    m_editMenu->addSeparator();

    QAction *preferencesAction = m_editMenu->addAction(tr("&Preferences..."), this, &MainWindow::showPreferences);
    preferencesAction->setShortcut(QKeySequence::Preferences);

    // ============================================================
    // Search menu
    // ============================================================
    m_searchMenu = menuBar()->addMenu(tr("&Search"));

    m_findAction = m_searchMenu->addAction(tr("&Find..."), this, &MainWindow::find);
    m_findAction->setShortcut(QKeySequence::Find);

    m_findNextAction = m_searchMenu->addAction(tr("Find &Next"), this, &MainWindow::findNext);
    m_findNextAction->setShortcut(QKeySequence::FindNext);

    m_findPrevAction = m_searchMenu->addAction(tr("Find &Previous"), this, &MainWindow::findPrevious);
    m_findPrevAction->setShortcut(QKeySequence::FindPrevious);

    m_replaceAction = m_searchMenu->addAction(tr("&Replace..."), this, &MainWindow::replace);
    m_replaceAction->setShortcut(QKeySequence::Replace);

    QAction *findInFilesAction = m_searchMenu->addAction(tr("Find in &Files..."));
    findInFilesAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F));
    connect(findInFilesAction, &QAction::triggered, this, [this]() {
        find();
        if (m_searchDialog) m_searchDialog->showTab(2);
    });

    QAction *incrementalSearchAction = m_searchMenu->addAction(tr("&Incremental Search"));
    incrementalSearchAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    connect(incrementalSearchAction, &QAction::triggered, this, &MainWindow::showIncrementalSearch);

    m_searchMenu->addSeparator();

    QAction *goToLineAction = m_searchMenu->addAction(tr("&Go to Line..."), this, &MainWindow::goToLineDialog);
    goToLineAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));

    // ============================================================
    // View menu
    // ============================================================
    m_viewMenu = menuBar()->addMenu(tr("&View"));

    m_wordWrapAction = m_viewMenu->addAction(tr("&Word Wrap"));
    m_wordWrapAction->setCheckable(true);
    m_wordWrapAction->setChecked(Settings::instance().wordWrap());
    connect(m_wordWrapAction, &QAction::toggled, this, &MainWindow::toggleWordWrap);

    QAction *lineNumbersAction = m_viewMenu->addAction(tr("&Line Numbers"));
    lineNumbersAction->setCheckable(true);
    lineNumbersAction->setChecked(Settings::instance().showLineNumbers());
    connect(lineNumbersAction, &QAction::toggled, this, &MainWindow::toggleLineNumbers);

    QAction *terminalAction = m_viewMenu->addAction(tr("&Terminal"));
    terminalAction->setCheckable(true);
    terminalAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Apostrophe));
    connect(terminalAction, &QAction::toggled, this, &MainWindow::toggleTerminal);

    m_viewMenu->addSeparator();

    m_zoomInAction = m_viewMenu->addAction(tr("Zoom &In"), this, &MainWindow::zoomIn);
    m_zoomInAction->setShortcut(QKeySequence::ZoomIn);

    m_zoomOutAction = m_viewMenu->addAction(tr("Zoom &Out"), this, &MainWindow::zoomOut);
    m_zoomOutAction->setShortcut(QKeySequence::ZoomOut);

    QAction *resetZoomAction = m_viewMenu->addAction(tr("&Reset Zoom"), this, &MainWindow::resetZoom);
    resetZoomAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));

    m_viewMenu->addSeparator();

    QAction *alwaysOnTopAction = m_viewMenu->addAction(tr("Always on &Top"));
    alwaysOnTopAction->setCheckable(true);
    connect(alwaysOnTopAction, &QAction::toggled, this, &MainWindow::toggleAlwaysOnTop);

    m_showWhitespaceAction = m_viewMenu->addAction(tr("Show &Whitespace"));
    m_showWhitespaceAction->setCheckable(true);
    connect(m_showWhitespaceAction, &QAction::toggled, this, &MainWindow::toggleWhitespace);

    m_showEOLAction = m_viewMenu->addAction(tr("Show &End of Line"));
    m_showEOLAction->setCheckable(true);
    connect(m_showEOLAction, &QAction::toggled, this, &MainWindow::toggleEndOfLine);

    m_showIndentGuideAction = m_viewMenu->addAction(tr("Show Indent &Guide"));
    m_showIndentGuideAction->setCheckable(true);
    connect(m_showIndentGuideAction, &QAction::toggled, this, &MainWindow::toggleIndentGuide);

    m_viewMenu->addSeparator();

    m_foldAllAction = m_viewMenu->addAction(tr("Fold All"), this, &MainWindow::foldAll);
    m_unfoldAllAction = m_viewMenu->addAction(tr("Unfold All"), this, &MainWindow::unfoldAll);

    m_viewMenu->addSeparator();

    m_viewMenu->addAction(tr("Summary..."), this, &MainWindow::showSummary);

    m_viewMenu->addSeparator();

    QAction *documentMapAction = m_viewMenu->addAction(tr("Document &Map"));
    documentMapAction->setCheckable(true);
    documentMapAction->setChecked(false);
    connect(documentMapAction, &QAction::toggled, m_documentMapDock, &QDockWidget::setVisible);
    connect(m_documentMapDock, &QDockWidget::visibilityChanged, documentMapAction, &QAction::setChecked);

    QAction *fullScreenAction = m_viewMenu->addAction(tr("&Full Screen"), this, &MainWindow::toggleFullScreen);
    fullScreenAction->setShortcut(QKeySequence::FullScreen);

    m_viewMenu->addSeparator();

    QAction *splitVertAction = m_viewMenu->addAction(tr("Split &Vertical"), this, &MainWindow::splitVertical);
    splitVertAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Backslash));

    QAction *splitHorizAction = m_viewMenu->addAction(tr("Split &Horizontal"), this, &MainWindow::splitHorizontal);
    splitHorizAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Backslash));

    m_viewMenu->addAction(tr("&Clone to Other View"), this, &MainWindow::cloneToOtherView);
    m_viewMenu->addAction(tr("&Move to Other View"), this, &MainWindow::moveToOtherView);

    QAction *closeSplitAction = m_viewMenu->addAction(tr("Close S&plit"), this, &MainWindow::closeSplit);
    closeSplitAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Backslash));

    QAction *focusOtherAction = m_viewMenu->addAction(tr("Focus &Other Split"), this, &MainWindow::focusOtherSplit);
    focusOtherAction->setShortcut(QKeySequence(Qt::Key_F6));

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
    m_languageMenu->setStyleSheet("QMenu { max-height: 600px; }");
    QAction *autoDetect = m_languageMenu->addAction(tr("Auto-detect"));
    connect(autoDetect, &QAction::triggered, this, [this]() {
        Editor *e = currentEditor();
        if (!e || !e->document()) return;
        QString detected = LanguageManager::instance().detectLanguage(
            e->document()->filePath(), e->toPlainText());
        e->setLanguage(detected);
    });
    m_languageMenu->addSeparator();

    // Group languages into categorized submenus to avoid one massive list
    QStringList langs = LanguageManager::instance().availableLanguages();
    std::sort(langs.begin(), langs.end());

    // Split into alphabetical submenus
    QMap<QChar, QMenu*> letterMenus;
    for (const QString &lang : langs) {
        QChar letter = lang.at(0).toUpper();
        if (!letterMenus.contains(letter)) {
            QString label = QString("%1...").arg(letter);
            letterMenus[letter] = m_languageMenu->addMenu(label);
        }
        QAction *a = letterMenus[letter]->addAction(lang);
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
    QAction *styleConfigAction = m_settingsMenu->addAction(tr("&Style Configurator..."));
    styleConfigAction->setEnabled(false);
    QAction *shortcutMapperAction = m_settingsMenu->addAction(tr("&Shortcut Mapper..."));
    shortcutMapperAction->setEnabled(false);

    // ============================================================
    // Tools menu (Command Palette only)
    // ============================================================
    m_toolsMenu = menuBar()->addMenu(tr("&Tools"));

    QAction *commandPaletteAction = m_toolsMenu->addAction(tr("Command &Palette..."), this, &MainWindow::showCommandPalette);
    commandPaletteAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));

    m_toolsMenu->addSeparator();
    QAction *evalAction = m_toolsMenu->addAction(
        tr("&Evaluate Selection"), this, &MainWindow::evaluateSelection);
    evalAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E));
    m_toolsMenu->addAction(
        tr("Configure &Endpoint..."), this, &MainWindow::configureEndpoint);
    m_toolsMenu->addSeparator();
    m_toolsMenu->addAction(
        tr("&Plugins..."), this, &MainWindow::showPluginDialog);

    // ============================================================
    // Macro menu
    // ============================================================
    m_macroMenu = menuBar()->addMenu(tr("&Macro"));
    m_startRecordingAction = m_macroMenu->addAction(tr("Start &Recording"), this, &MainWindow::startRecordingMacro, QKeySequence(Qt::Key_F9));
    m_stopRecordingAction = m_macroMenu->addAction(tr("&Stop Recording"), this, &MainWindow::stopRecordingMacro, QKeySequence(Qt::SHIFT | Qt::Key_F9));
    m_playbackAction = m_macroMenu->addAction(tr("&Playback"), this, &MainWindow::playbackMacro, QKeySequence(Qt::Key_F10));
    m_macroMenu->addAction(tr("Run &Multiple Times..."), this, &MainWindow::runMacroMultipleTimes);
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
    // Git menu
    // ============================================================
    m_gitMenu = menuBar()->addMenu(tr("&Git"));

    QAction *gitCommitAction = m_gitMenu->addAction(
        tr("&Commit..."), this, &MainWindow::gitCommit);
    gitCommitAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G));

    m_gitMenu->addAction(
        tr("Switch &Branch..."), this, &MainWindow::gitBranchSwitch);

    m_gitMenu->addSeparator();
    m_gitMenu->addAction(tr("&Refresh Branch"), this, &MainWindow::updateGitBranch);

    // ============================================================
    // Plugins menu
    // ============================================================
    QMenu *pluginsMenu = menuBar()->addMenu(tr("&Plugins"));

    QStringList pluginNames = PluginManager::instance().pluginNames();
    if (pluginNames.isEmpty()) {
        pluginsMenu->addAction(tr("(No plugins loaded)"))->setEnabled(false);
    } else {
        for (const QString &name : pluginNames) {
            QAction *action = pluginsMenu->addAction(name);
            action->setCheckable(true);
            action->setChecked(true);
        }
    }

    pluginsMenu->addSeparator();
    pluginsMenu->addAction(tr("Open Plugin &Directory..."), this, [this]() {
        QString dir = QStandardPaths::writableLocation(
            QStandardPaths::AppConfigLocation) + "/plugins";
        QDir().mkpath(dir);
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    });

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

    m_helpMenu->addAction(tr("&About PrimeEdit"), this, &MainWindow::showAbout);
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

    // Git branch indicator in status bar
    m_gitBranchLabel = new QLabel(this);
    m_gitBranchLabel->setStyleSheet(
        "QLabel { padding: 0 8px; color: #999; }");
    m_statusBar->addPermanentWidget(m_gitBranchLabel);
    updateGitBranch();
}

void MainWindow::setupShortcuts()
{
    // Bracket matching: Ctrl+B jumps to matching bracket
    QShortcut *bracketShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_B), this);
    connect(bracketShortcut, &QShortcut::activated, this, [this]() {
        Editor *e = currentEditor();
        if (e) e->jumpToMatchingBracket();
    });

    // Go to Definition (F12)
    QShortcut *gotoDefShortcut = new QShortcut(QKeySequence(Qt::Key_F12), this);
    connect(gotoDefShortcut, &QShortcut::activated,
            this, &MainWindow::gotoDefinition);

    // Find References (Shift+F12)
    QShortcut *refsShortcut = new QShortcut(
        QKeySequence(Qt::SHIFT | Qt::Key_F12), this);
    connect(refsShortcut, &QShortcut::activated,
            this, &MainWindow::findReferences);

    // Rename Symbol (F2)
    QShortcut *renameShortcut = new QShortcut(QKeySequence(Qt::Key_F2), this);
    connect(renameShortcut, &QShortcut::activated,
            this, &MainWindow::renameSymbol);

    // Hadith validation (Ctrl+Shift+H)
    QShortcut *hadithShortcut = new QShortcut(
        QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H), this);
    connect(hadithShortcut, &QShortcut::activated,
            this, &MainWindow::validateHadith);
}

void MainWindow::newFile()
{
    Document *doc = DocumentManager::instance().createDocument();
    Editor *editor = createEditor(doc);

    QString title = tr("Untitled %1").arg(++m_untitledCounter);
    int index = m_tabWidget->addTab(editor, title);
    m_tabWidget->setTabIcon(index, TabBar::iconForFile(QString()));
    m_tabWidget->setCurrentIndex(index);

    if (m_lspBridge) m_lspBridge->onEditorOpened(editor);

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

    // Check if already open in either split
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        Editor *ed = qobject_cast<Editor*>(m_tabWidget->widget(i));
        if (ed && ed->document() && ed->document()->filePath() == path) {
            m_tabWidget->setCurrentIndex(i);
            m_tabWidget->currentWidget()->setFocus();
            return;
        }
    }
    if (m_tabWidget2) {
        for (int i = 0; i < m_tabWidget2->count(); ++i) {
            Editor *ed = qobject_cast<Editor*>(m_tabWidget2->widget(i));
            if (ed && ed->document() && ed->document()->filePath() == path) {
                m_tabWidget2->setCurrentIndex(i);
                m_tabWidget2->currentWidget()->setFocus();
                return;
            }
        }
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

    if (m_lspBridge) m_lspBridge->onEditorOpened(editor);

    PluginManager::instance().broadcastFileOpened(path);

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
        PluginManager::instance().broadcastFileSaved(doc->filePath());
        updateWindowTitle();
        updateTabTextForDocument(doc);

        // Auto-upload if this is a remote file
        QString localPath = doc->filePath();
        if (m_remoteFiles.contains(localPath)) {
            uploadRemoteFile(localPath);
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
        updateTabTextForDocument(doc);
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Could not save file."));
    }
}

void MainWindow::saveAllFiles()
{
    auto saveInWidget = [this](TabWidget *tw) {
        if (!tw) return;
        for (int i = 0; i < tw->count(); ++i) {
            Editor *editor = qobject_cast<Editor*>(tw->widget(i));
            if (editor && editor->document() && editor->document()->isModified()) {
                tw->setCurrentIndex(i);
                editor->setFocus();
                saveFile();
            }
        }
    };
    saveInWidget(m_tabWidget);
    saveInWidget(m_tabWidget2);
}

void MainWindow::reloadFromDisk()
{
    Editor *editor = currentEditor();
    if (!editor || !editor->document() || editor->document()->isUntitled()) {
        return;
    }

    if (editor->document()->isModified()) {
        int ret = QMessageBox::question(
            this, tr("Reload"),
            tr("Are you sure? Unsaved changes will be lost."),
            QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes) {
            return;
        }
    }

    QString filePath = editor->document()->filePath();
    editor->document()->load(filePath);
    editor->syncFromDocument();
    updateWindowTitle();
}

bool MainWindow::closeFile(int index)
{
    TabWidget *tw = activeTabWidget();

    if (index < 0) {
        index = tw->currentIndex();
    }

    if (index < 0 || index >= tw->count()) {
        return false;
    }

    Editor *editor = qobject_cast<Editor*>(tw->widget(index));
    if (!editor) {
        return false;
    }

    Document *doc = editor->document();
    if (doc && doc->isModified() && !doc->isReadOnly()) {
        QMessageBox::StandardButton result = QMessageBox::question(
            this, tr("Save Changes"),
            tr("Do you want to save changes to '%1'?").arg(doc->displayName()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (result == QMessageBox::Cancel) {
            return false;
        }
        if (result == QMessageBox::Save) {
            tw->setCurrentIndex(index);
            editor->setFocus();
            saveFile();
            if (doc->isModified()) {
                return false;  // Save failed or canceled
            }
        }
    }

    const QString closedPath = doc ? doc->filePath() : QString();

    tw->removeTab(index);
    editor->deleteLater();
    if (doc) {
        DocumentManager::instance().closeDocument(doc, true);
    }

    if (!closedPath.isEmpty()) {
        PluginManager::instance().broadcastFileClosed(closedPath);
    }

    // Auto-unsplit if split 2 is now empty
    if (tw == m_tabWidget2) {
        checkUnsplitNeeded();
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

    // Close split 2 tabs first (unsplits automatically when empty)
    if (m_tabWidget2 && m_tabWidget2->count() > 0) {
        // Focus split 2 so closeFile operates on it
        if (m_tabWidget2->currentWidget())
            m_tabWidget2->currentWidget()->setFocus();
        while (m_tabWidget2 && m_tabWidget2->count() > 0) {
            if (!closeFile(0)) {
                m_closingAll = false;
                return false;
            }
        }
    }

    // Close split 1 tabs
    if (m_tabWidget->currentWidget())
        m_tabWidget->currentWidget()->setFocus();
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
    return qobject_cast<Editor*>(activeTabWidget()->currentWidget());
}

QStringList MainWindow::openFilePaths() const
{
    QStringList paths;
    auto collectFromWidget = [&](TabWidget *tw) {
        if (!tw) return;
        for (int i = 0; i < tw->count(); ++i) {
            Editor *editor = qobject_cast<Editor*>(tw->widget(i));
            if (editor && editor->document() && !editor->document()->isUntitled()) {
                paths.append(editor->document()->filePath());
            }
        }
    };
    collectFromWidget(m_tabWidget);
    collectFromWidget(m_tabWidget2);
    return paths;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Notepad++ behavior: silently persist ALL unsaved documents.
    // No save prompts on application quit. Everything is restored on next launch.

    // Save unsaved documents FIRST — most critical data
    Session::instance().saveUnsavedDocuments(this);
    Session::instance().save(this);
    Settings::instance().save();
    m_closingAll = true;
    event->accept();
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
    Q_UNUSED(index);
    updateWindowTitle();
    updateStatusBar();
    updateMenuState();

    if (m_documentMap) {
        m_documentMap->setEditor(currentEditor());
    }

    if (m_functionListDock && m_functionListDock->isVisible() && m_functionList) {
        m_functionList->setEditor(currentEditor());
    }

    if (m_lspBridge) {
        m_lspBridge->onEditorChanged(currentEditor());
    }

    updateGitBranch();
}

void MainWindow::onTabCloseRequested(int index)
{
    closeFile(index);
}

void MainWindow::onDocumentModified(bool modified)
{
    Q_UNUSED(modified);
    Document *doc = qobject_cast<Document*>(sender());
    if (!doc) {
        return;
    }

    updateTabTextForDocument(doc);
    updateWindowTitle();
    updateMenuState();
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
    if (!editor) return;

    QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, &QPrintPreviewDialog::paintRequested, this, [this, editor](QPrinter *p) {
        printDocument(p, editor);
    });
    preview.exec();
}

void MainWindow::printDocument(QPrinter *printer, Editor *editor)
{
    if (!printer || !editor) return;

    QTextDocument *doc = editor->QPlainTextEdit::document();
    if (!doc) return;

    const double margin = 0.75 * 72.0;  // 0.75 inches in points

    printer->setPageMargins(QMarginsF(margin, margin, margin, margin), QPageLayout::Point);

    const QRectF pageRect = printer->pageRect(QPrinter::DevicePixel);
    const double pageWidth = pageRect.width();
    const double pageHeight = pageRect.height();

    QFont textFont = editor->font();
    textFont.setPointSize(10);
    QFont headerFont = textFont;
    headerFont.setBold(true);
    QFont lineNumFont = textFont;
    lineNumFont.setPointSize(9);

    QFontMetricsF textFm(textFont);
    QFontMetricsF headerFm(headerFont);
    QFontMetricsF lineNumFm(lineNumFont);

    const double lineHeight = textFm.height();
    const double headerHeight = headerFm.height() * 2.0;
    const double footerHeight = textFm.height() * 2.0;
    const double contentHeight = pageHeight - headerHeight - footerHeight;
    const int linesPerPage = static_cast<int>(contentHeight / lineHeight);

    int totalLines = doc->blockCount();
    double lineNumWidth = lineNumFm.horizontalAdvance(QString::number(totalLines)) + 20;
    double textStartX = lineNumWidth + 10;

    QString fileName = editor->document()->fileName();
    if (fileName.isEmpty()) fileName = tr("Untitled");
    QString dateStr = QDate::currentDate().toString(QLocale().dateFormat(QLocale::LongFormat));

    int totalPages = (totalLines + linesPerPage - 1) / linesPerPage;
    if (totalPages < 1) totalPages = 1;

    QPainter painter;
    if (!painter.begin(printer)) return;

    QTextBlock block = doc->begin();

    for (int page = 0; page < totalPages; ++page) {
        if (page > 0) printer->newPage();

        // Header: filename left, date right
        painter.setFont(headerFont);
        painter.setPen(Qt::black);
        QRectF headerRect(0, 0, pageWidth, headerFm.height());
        painter.drawText(headerRect, Qt::AlignLeft | Qt::AlignVCenter, fileName);
        painter.drawText(headerRect, Qt::AlignRight | Qt::AlignVCenter, dateStr);
        double headerLineY = headerFm.height() + 4;
        painter.drawLine(QPointF(0, headerLineY), QPointF(pageWidth, headerLineY));

        // Footer: page number centered
        painter.setFont(textFont);
        painter.setPen(Qt::black);
        QString footerText = tr("Page %1 of %2").arg(page + 1).arg(totalPages);
        QRectF footerRect(0, pageHeight - textFm.height(), pageWidth, textFm.height());
        painter.drawText(footerRect, Qt::AlignCenter, footerText);
        double footerLineY = pageHeight - footerHeight + 4;
        painter.drawLine(QPointF(0, footerLineY), QPointF(pageWidth, footerLineY));

        // Content lines
        double y = headerHeight;

        for (int i = 0; i < linesPerPage && block.isValid(); ++i, block = block.next()) {
            int lineNum = page * linesPerPage + i + 1;

            // Line number (right-aligned, gray)
            painter.setFont(lineNumFont);
            painter.setPen(Qt::gray);
            QRectF lineNumRect(0, y, lineNumWidth, lineHeight);
            painter.drawText(lineNumRect, Qt::AlignRight | Qt::AlignVCenter,
                             QString::number(lineNum));

            // Syntax-highlighted text via block layout formats
            QTextLayout *layout = block.layout();
            QString lineText = block.text();
            double x = textStartX;

            if (!layout || layout->formats().isEmpty()) {
                painter.setFont(textFont);
                painter.setPen(Qt::black);
                painter.drawText(QPointF(x, y + textFm.ascent()), lineText);
            } else {
                QVector<QTextLayout::FormatRange> formats = layout->formats();
                int pos = 0;
                for (const QTextLayout::FormatRange &fmt : formats) {
                    if (fmt.start > pos) {
                        painter.setFont(textFont);
                        painter.setPen(Qt::black);
                        QString seg = lineText.mid(pos, fmt.start - pos);
                        painter.drawText(QPointF(x, y + textFm.ascent()), seg);
                        x += textFm.horizontalAdvance(seg);
                    }

                    QFont segFont = textFont;
                    if (fmt.format.fontWeight() > QFont::Normal)
                        segFont.setBold(true);
                    if (fmt.format.fontItalic())
                        segFont.setItalic(true);
                    painter.setFont(segFont);

                    QColor fg = fmt.format.foreground().color();
                    if (!fg.isValid()) fg = Qt::black;
                    painter.setPen(fg);

                    int end = qMin(fmt.start + fmt.length,
                                   static_cast<int>(lineText.length()));
                    QString seg = lineText.mid(fmt.start, end - fmt.start);
                    painter.drawText(QPointF(x, y + textFm.ascent()), seg);
                    QFontMetricsF segFm(segFont);
                    x += segFm.horizontalAdvance(seg);
                    pos = end;
                }
                if (pos < lineText.length()) {
                    painter.setFont(textFont);
                    painter.setPen(Qt::black);
                    painter.drawText(QPointF(x, y + textFm.ascent()),
                                     lineText.mid(pos));
                }
            }

            y += lineHeight;
        }
    }

    painter.end();
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
    if (!m_searchDialog) {
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
        connect(m_searchDialog, &SearchDialog::replaceAll, this, [this]() {
            Editor *e = currentEditor();
            if (!e || !m_searchDialog) return;
            SearchEngine engine;
            QString result = engine.replaceInText(
                e->toPlainText(), m_searchDialog->searchText(),
                m_searchDialog->replaceText(), m_searchDialog->searchOptions());
            e->selectAll();
            e->insertPlainText(result);
        });
        connect(m_searchDialog, &SearchDialog::countRequested, this, &MainWindow::onCountRequested);
        connect(m_searchDialog, &SearchDialog::findAllInCurrent, this, &MainWindow::onFindAllInCurrent);
        connect(m_searchDialog, &SearchDialog::findAllInAllOpen, this, &MainWindow::onFindAllInAllOpen);
        connect(m_searchDialog, &SearchDialog::findInFiles, this, &MainWindow::onFindInFiles);
        connect(m_searchDialog, &SearchDialog::markAll, this, [this]() {
            Editor *e = currentEditor();
            if (e && m_searchDialog) {
                e->markAll(m_searchDialog->searchText(), m_searchDialog->searchOptions());
            }
        });
        connect(m_searchDialog, &SearchDialog::clearMarks, this, [this]() {
            Editor *e = currentEditor();
            if (e) e->clearMarks();
        });
    }
    Editor *e = currentEditor();
    if (e && !e->selectedText().isEmpty()) {
        m_searchDialog->setSearchText(e->selectedText());
    }
    m_searchDialog->showTab(0);
    m_searchDialog->show();
    m_searchDialog->raise();
    m_searchDialog->activateWindow();
}

void MainWindow::findNext()
{
    Editor *editor = currentEditor();
    if (!editor) return;

    QString pattern;
    if (m_searchDialog) {
        pattern = m_searchDialog->searchText();
    } else if (m_incrementalSearchBar && m_incrementalSearchBar->isVisible()) {
        pattern = m_incrementalSearchBar->searchText();
    }
    if (pattern.isEmpty()) return;

    editor->findNext(pattern, QTextDocument::FindFlags());
}

void MainWindow::findPrevious()
{
    Editor *editor = currentEditor();
    if (!editor) return;

    QString pattern;
    if (m_searchDialog) {
        pattern = m_searchDialog->searchText();
    } else if (m_incrementalSearchBar && m_incrementalSearchBar->isVisible()) {
        pattern = m_incrementalSearchBar->searchText();
    }
    if (pattern.isEmpty()) return;

    editor->findPrevious(pattern, QTextDocument::FindFlags());
}

void MainWindow::replace()
{
    if (!m_searchDialog) {
        find();
    }
    if (m_searchDialog) {
        Editor *e = currentEditor();
        if (e && !e->selectedText().isEmpty()) {
            m_searchDialog->setSearchText(e->selectedText());
        }
        m_searchDialog->showTab(1);
        m_searchDialog->show();
        m_searchDialog->raise();
        m_searchDialog->activateWindow();
    }
}

void MainWindow::showIncrementalSearch()
{
    m_incrementalSearchBar->activate();
}

void MainWindow::onSearchResultActivated(const QString &filePath, int line)
{
    if (!filePath.isEmpty()) {
        openFile(filePath);
    }
    Editor *e = currentEditor();
    if (e) {
        e->goToLine(line);
    }
}

void MainWindow::onFindAllInCurrent()
{
    Editor *e = currentEditor();
    if (!e || !m_searchDialog) return;

    SearchEngine engine;
    auto results = engine.findAll(e->toPlainText(),
                                  m_searchDialog->searchText(),
                                  m_searchDialog->searchOptions());

    m_searchResultsPanel->clear();
    m_searchResultsPanel->setHeader(m_searchDialog->searchText(),
                                     results.size(), 1);
    for (const auto &r : results) {
        m_searchResultsPanel->addResult(r);
    }
    m_searchResultsDock->show();
}

void MainWindow::onFindAllInAllOpen()
{
    if (!m_searchDialog) return;

    SearchEngine engine;
    m_searchResultsPanel->clear();

    int totalHits = 0;
    int totalFiles = 0;

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        Editor *e = qobject_cast<Editor*>(m_tabWidget->widget(i));
        if (!e) continue;

        auto results = engine.findAll(e->toPlainText(),
                                      m_searchDialog->searchText(),
                                      m_searchDialog->searchOptions());
        if (!results.isEmpty()) {
            totalFiles++;
            totalHits += results.size();
            for (auto r : results) {
                r.filePath = m_tabWidget->tabText(i);
                m_searchResultsPanel->addResult(r);
            }
        }
    }

    m_searchResultsPanel->setHeader(m_searchDialog->searchText(),
                                     totalHits, totalFiles);
    m_searchResultsDock->show();
}

void MainWindow::onFindInFiles()
{
    if (!m_searchDialog) return;

    SearchEngine *engine = new SearchEngine(this);

    m_searchResultsPanel->clear();
    m_searchResultsDock->show();

    connect(engine, &SearchEngine::fileSearchResult, this, [this](const SearchResult &result) {
        m_searchResultsPanel->addResult(result);
    });

    connect(engine, &SearchEngine::fileSearchFinished, this,
            [this, engine](int totalHits, int totalFiles) {
        m_searchResultsPanel->setHeader(m_searchDialog ? m_searchDialog->searchText() : QString(),
                                         totalHits, totalFiles);
        engine->deleteLater();
    });

    engine->findInFiles(m_searchDialog->directory(),
                        m_searchDialog->searchText(),
                        m_searchDialog->fileFilter(),
                        m_searchDialog->searchOptions(),
                        m_searchDialog->recursive(),
                        m_searchDialog->includeHidden());
}

void MainWindow::onCountRequested()
{
    Editor *e = currentEditor();
    if (!e || !m_searchDialog) return;

    SearchEngine engine;
    int count = engine.matchCount(e->toPlainText(),
                                   m_searchDialog->searchText(),
                                   m_searchDialog->searchOptions());
    QMessageBox::information(this, tr("Count"),
                             tr("%1 matches found").arg(count));
}

void MainWindow::onIncrementalSearchChanged(const QString &text)
{
    Editor *e = currentEditor();
    if (!e || text.isEmpty()) return;
    e->findNext(text, QTextDocument::FindFlags());
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
    QMessageBox::about(this, tr("About PrimeEdit"),
        tr("<h2>PrimeEdit 1.0</h2>"
           "<p>A native editor for structured documents, where content is "
           "validated, enforced, and executable.</p>"
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

void MainWindow::runMacroMultipleTimes()
{
    MacroDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;

    Editor *editor = currentEditor();
    if (!editor) return;

    if (dialog.runUntilEnd()) {
        int lastLine = static_cast<QPlainTextEdit*>(editor)->document()->blockCount();
        while (editor->currentLine() < lastLine) {
            int prevLine = editor->currentLine();
            MacroRecorder::instance().playback(editor);
            if (editor->currentLine() == prevLine) break;
        }
    } else {
        for (int i = 0; i < dialog.runCount(); ++i) {
            MacroRecorder::instance().playback(editor);
        }
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
        title += " - PrimeEdit";
        setWindowTitle(title);
    } else {
        setWindowTitle("PrimeEdit");
    }
}

void MainWindow::updateStatusBar()
{
    Editor *editor = currentEditor();
    if (editor) {
        m_statusBar->updateFromEditor(editor);
    } else {
        m_statusBar->clear();
    }
}

void MainWindow::updateMenuState()
{
    Editor *editor = currentEditor();
    bool hasEditor = (editor != nullptr);
    bool hasSelection = hasEditor && editor->textCursor().hasSelection();
    bool isModified = hasEditor && editor->document() && editor->document()->isModified();

    // Undo/redo state from QTextDocument
    bool canUndo = false;
    bool canRedo = false;
    if (hasEditor) {
        QTextDocument *textDoc = static_cast<QPlainTextEdit*>(editor)->document();
        if (textDoc) {
            canUndo = textDoc->isUndoAvailable();
            canRedo = textDoc->isRedoAvailable();
        }
    }

    // File operations
    if (m_saveAction) m_saveAction->setEnabled(isModified);
    if (m_saveAllAction) {
        bool anyModified = false;
        auto checkModified = [&](TabWidget *tw) {
            if (!tw) return;
            for (int i = 0; i < tw->count(); ++i) {
                Editor *e = qobject_cast<Editor*>(tw->widget(i));
                if (e && e->document() && e->document()->isModified()) {
                    anyModified = true;
                    return;
                }
            }
        };
        checkModified(m_tabWidget);
        checkModified(m_tabWidget2);
        m_saveAllAction->setEnabled(anyModified);
    }
    if (m_closeAction) m_closeAction->setEnabled(hasEditor);
    if (m_printAction) m_printAction->setEnabled(hasEditor);

    // Edit operations
    if (m_undoAction) m_undoAction->setEnabled(canUndo);
    if (m_redoAction) m_redoAction->setEnabled(canRedo);
    if (m_cutAction) m_cutAction->setEnabled(hasSelection);
    if (m_copyAction) m_copyAction->setEnabled(hasSelection);
    if (m_pasteAction) m_pasteAction->setEnabled(hasEditor && !QApplication::clipboard()->text().isEmpty());

    // Search
    if (m_findAction) m_findAction->setEnabled(hasEditor);
    if (m_replaceAction) m_replaceAction->setEnabled(hasEditor);

    // View
    if (m_zoomInAction) m_zoomInAction->setEnabled(hasEditor);
    if (m_zoomOutAction) m_zoomOutAction->setEnabled(hasEditor);

    // Toggle state sync on tab change
    if (m_wordWrapAction && hasEditor)
        m_wordWrapAction->setChecked(editor->wordWrapEnabled());
    if (m_showWhitespaceAction && hasEditor)
        m_showWhitespaceAction->setChecked(editor->showWhitespace());
    if (m_showEOLAction && hasEditor)
        m_showEOLAction->setChecked(editor->showEOL());
    if (m_showIndentGuideAction && hasEditor)
        m_showIndentGuideAction->setChecked(editor->showIndentGuide());

    // Bookmarks
    if (m_toggleBookmarkAction) m_toggleBookmarkAction->setEnabled(hasEditor);
    if (m_nextBookmarkAction) m_nextBookmarkAction->setEnabled(hasEditor);
    if (m_prevBookmarkAction) m_prevBookmarkAction->setEnabled(hasEditor);
    if (m_clearBookmarksAction) m_clearBookmarksAction->setEnabled(hasEditor);

    // Folding
    bool canFold = hasEditor && editor->foldingEnabled();
    if (m_foldAllAction) m_foldAllAction->setEnabled(canFold);
    if (m_unfoldAllAction) m_unfoldAllAction->setEnabled(canFold);

    // Line operations
    if (m_toggleCommentAction) m_toggleCommentAction->setEnabled(hasEditor);
    if (m_duplicateLineAction) m_duplicateLineAction->setEnabled(hasEditor);
    if (m_deleteLineAction) m_deleteLineAction->setEnabled(hasEditor);

    // Menus
    if (m_encodingMenu) m_encodingMenu->setEnabled(hasEditor);
    if (m_languageMenu) m_languageMenu->setEnabled(hasEditor);
}

Editor *MainWindow::createEditor(Document *document)
{
    Editor *editor = new Editor(document, this);

    connect(document, &Document::modifiedChanged, this, &MainWindow::onDocumentModified);
    connect(editor, &Editor::cursorPositionChanged, this, &MainWindow::onCursorPositionChanged);

    // Forward editor events to plugin system
    connect(editor, &QPlainTextEdit::textChanged, this, []() {
        PluginManager::instance().broadcastTextChanged();
    });
    connect(editor, &QPlainTextEdit::selectionChanged, this, [editor]() {
        PluginManager::instance().broadcastSelectionChanged(
            editor->textCursor().selectedText());
    });

    // Undo/redo state tracking
    QTextDocument *textDoc = static_cast<QPlainTextEdit*>(editor)->document();
    connect(textDoc, &QTextDocument::undoAvailable, this, [this](bool available) {
        if (m_undoAction) m_undoAction->setEnabled(available);
    });
    connect(textDoc, &QTextDocument::redoAvailable, this, [this](bool available) {
        if (m_redoAction) m_redoAction->setEnabled(available);
    });

    // Selection state for Cut/Copy
    connect(editor, &QPlainTextEdit::selectionChanged, this, [this]() {
        Editor *e = currentEditor();
        bool hasSel = e && e->textCursor().hasSelection();
        if (m_cutAction) m_cutAction->setEnabled(hasSel);
        if (m_copyAction) m_copyAction->setEnabled(hasSel);
    });

    return editor;
}

int MainWindow::findEditorIndex(Editor *editor) const
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabWidget->widget(i) == editor) {
            return i;
        }
    }
    if (m_tabWidget2) {
        for (int i = 0; i < m_tabWidget2->count(); ++i) {
            if (m_tabWidget2->widget(i) == editor) {
                return i;
            }
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
    if (m_tabWidget2) {
        for (int i = 0; i < m_tabWidget2->count(); ++i) {
            Editor *editor = qobject_cast<Editor*>(m_tabWidget2->widget(i));
            if (editor && editor->document() == document) {
                return i;
            }
        }
    }
    return -1;
}

void MainWindow::updateTabTextForDocument(Document *doc)
{
    if (!doc) return;
    auto updateInWidget = [&](TabWidget *tw) {
        if (!tw) return;
        for (int i = 0; i < tw->count(); ++i) {
            Editor *e = qobject_cast<Editor*>(tw->widget(i));
            if (e && e->document() == doc) {
                QString title = doc->displayName();
                if (doc->isModified()) title += " *";
                tw->setTabText(i, title);

                // Update icon: tint red when modified, restore when saved
                QString filePath = doc->filePath();
                if (doc->isModified()) {
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
                    tw->setTabIcon(i, QIcon(px));
                } else {
                    tw->setTabIcon(i, TabBar::iconForFile(filePath));
                }
            }
        }
    };
    updateInWidget(m_tabWidget);
    updateInWidget(m_tabWidget2);
}

// ============================================================
// Split view implementation
// ============================================================

TabWidget *MainWindow::activeTabWidget() const
{
    if (m_tabWidget2) {
        // Try focus-based detection first
        QWidget *focused = QApplication::focusWidget();
        if (focused) {
            QWidget *parent = focused;
            while (parent) {
                if (parent == m_tabWidget2) return m_tabWidget2;
                if (parent == m_tabWidget) return m_tabWidget;
                parent = parent->parentWidget();
            }
        }
        // Fall back to last known active (handles menus/dialogs)
        if (m_lastActiveTabWidget && m_lastActiveTabWidget == m_tabWidget2)
            return m_tabWidget2;
    }
    return m_tabWidget;
}

TabWidget *MainWindow::inactiveTabWidget() const
{
    if (!m_tabWidget2) return nullptr;
    return (activeTabWidget() == m_tabWidget) ? m_tabWidget2 : m_tabWidget;
}

void MainWindow::connectTabWidget(TabWidget *tw)
{
    if (tw == m_tabWidget) {
        connect(tw, &QTabWidget::currentChanged,
                this, &MainWindow::onTabChanged);
        connect(tw, &QTabWidget::tabCloseRequested,
                this, &MainWindow::onTabCloseRequested);
    } else {
        connect(tw, &QTabWidget::currentChanged,
                this, &MainWindow::onTab2Changed);
        connect(tw, &QTabWidget::tabCloseRequested,
                this, &MainWindow::onTab2CloseRequested);
    }
}

void MainWindow::splitView(Qt::Orientation orientation)
{
    if (m_tabWidget2) {
        // Already split — just change orientation
        m_splitter->setOrientation(orientation);
        return;
    }

    m_splitter->setOrientation(orientation);

    m_tabWidget2 = new TabWidget(this);
    m_splitter->addWidget(m_tabWidget2);
    connectTabWidget(m_tabWidget2);

    // Clone current document into the new split
    Editor *current = qobject_cast<Editor*>(m_tabWidget->currentWidget());
    if (current && current->document()) {
        Editor *cloned = createEditor(current->document());
        QString title = current->document()->displayName();
        int idx = m_tabWidget2->addTab(cloned, title);
        m_tabWidget2->setTabIcon(idx,
            TabBar::iconForFile(current->document()->filePath()));
        m_tabWidget2->setCurrentIndex(idx);
    }
}

void MainWindow::unsplit()
{
    if (!m_tabWidget2) return;

    // Move all tabs from m_tabWidget2 to m_tabWidget
    while (m_tabWidget2->count() > 0) {
        QWidget *w = m_tabWidget2->widget(0);
        QString title = m_tabWidget2->tabText(0);
        QIcon icon = m_tabWidget2->tabIcon(0);
        m_tabWidget2->removeTab(0);
        int idx = m_tabWidget->addTab(w, icon, title);
        Q_UNUSED(idx);
    }

    delete m_tabWidget2;
    m_tabWidget2 = nullptr;

    m_tabWidget->setFocus();
    updateWindowTitle();
}

void MainWindow::splitVertical()
{
    splitView(Qt::Horizontal); // side by side
}

void MainWindow::splitHorizontal()
{
    splitView(Qt::Vertical); // one above other
}

void MainWindow::closeSplit()
{
    unsplit();
}

void MainWindow::focusOtherSplit()
{
    if (!m_tabWidget2) return;

    TabWidget *other = inactiveTabWidget();
    if (other && other->currentWidget()) {
        other->currentWidget()->setFocus();
    }
}

void MainWindow::onTab2Changed(int index)
{
    Q_UNUSED(index);
    // Only update if the second tab widget is focused
    if (activeTabWidget() == m_tabWidget2) {
        updateWindowTitle();
        updateStatusBar();
        updateMenuState();

        if (m_documentMap) {
            m_documentMap->setEditor(currentEditor());
        }
        if (m_functionListDock && m_functionListDock->isVisible()
            && m_functionList) {
            m_functionList->setEditor(currentEditor());
        }
        if (m_lspBridge) {
            m_lspBridge->onEditorChanged(currentEditor());
        }
    }
}

void MainWindow::onTab2CloseRequested(int index)
{
    if (!m_tabWidget2 || index < 0
        || index >= m_tabWidget2->count()) {
        return;
    }

    Editor *editor = qobject_cast<Editor*>(
        m_tabWidget2->widget(index));
    if (!editor) return;

    Document *doc = editor->document();

    // Check if this document has another editor in m_tabWidget
    bool hasOtherEditor = false;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        Editor *e = qobject_cast<Editor*>(m_tabWidget->widget(i));
        if (e && e->document() == doc) {
            hasOtherEditor = true;
            break;
        }
    }

    // If no other editor has this doc, prompt to save
    if (!hasOtherEditor && doc && doc->isModified()
        && !doc->isReadOnly()) {
        QMessageBox::StandardButton result = QMessageBox::question(
            this, tr("Save Changes"),
            tr("Do you want to save changes to '%1'?")
                .arg(doc->displayName()),
            QMessageBox::Save | QMessageBox::Discard
                | QMessageBox::Cancel);
        if (result == QMessageBox::Cancel) return;
        if (result == QMessageBox::Save) {
            m_tabWidget2->setCurrentIndex(index);
            saveFile();
            if (doc->isModified()) return;
        }
    }

    m_tabWidget2->removeTab(index);
    if (editor) editor->deleteLater();

    // If the document has no other editors, close it
    if (!hasOtherEditor) {
        DocumentManager::instance().closeDocument(doc, true);
    }

    checkUnsplitNeeded();
}

void MainWindow::checkUnsplitNeeded()
{
    if (m_tabWidget2 && m_tabWidget2->count() == 0) {
        unsplit();
    }
}

// ================================================================
// Plugin system
// ================================================================

/**
 * Concrete EditorAPI that delegates to MainWindow's current editor.
 */
class MainWindowEditorAPI : public EditorAPI
{
public:
    explicit MainWindowEditorAPI(MainWindow *mw) : m_mw(mw) {}

    QString currentText() const override
    {
        Editor *ed = m_mw->currentEditor();
        return ed ? ed->toPlainText() : QString();
    }

    void setText(const QString &text) override
    {
        Editor *ed = m_mw->currentEditor();
        if (ed) {
            ed->selectAll();
            ed->insertPlainText(text);
        }
    }

    void insertText(const QString &text) override
    {
        Editor *ed = m_mw->currentEditor();
        if (ed) {
            ed->insertPlainText(text);
        }
    }

    QString selectedText() const override
    {
        Editor *ed = m_mw->currentEditor();
        return ed ? ed->selectedText() : QString();
    }

    void replaceSelection(const QString &text) override
    {
        Editor *ed = m_mw->currentEditor();
        if (ed) {
            ed->replaceSelection(text);
        }
    }

    int cursorPosition() const override
    {
        Editor *ed = m_mw->currentEditor();
        return ed ? ed->textCursor().position() : 0;
    }

    void setCursorPosition(int pos) override
    {
        Editor *ed = m_mw->currentEditor();
        if (ed) {
            QTextCursor tc = ed->textCursor();
            tc.setPosition(pos);
            ed->setTextCursor(tc);
        }
    }

    int cursorLine() const override
    {
        Editor *ed = m_mw->currentEditor();
        return ed ? ed->currentLine() : 0;
    }

    int cursorColumn() const override
    {
        Editor *ed = m_mw->currentEditor();
        return ed ? ed->currentColumn() : 0;
    }

    QString currentFilePath() const override
    {
        Editor *ed = m_mw->currentEditor();
        if (ed && ed->document()) {
            return ed->document()->filePath();
        }
        return QString();
    }

    QString currentLanguage() const override
    {
        Editor *ed = m_mw->currentEditor();
        return ed ? ed->language() : QString();
    }

    void openFile(const QString &path) override
    {
        m_mw->openFile(path);
    }

    void saveCurrentFile() override
    {
        m_mw->saveFile();
    }

    void addMenuItem(const QString &menuPath, const QString &label,
                     std::function<void()> callback) override
    {
        Q_UNUSED(menuPath);
        Q_UNUSED(label);
        Q_UNUSED(callback);
        // TODO(PLUGIN-1): Implement dynamic menu registration
    }

    void addToolBarButton(const QString &iconPath, const QString &tooltip,
                          std::function<void()> callback) override
    {
        Q_UNUSED(iconPath);
        Q_UNUSED(tooltip);
        Q_UNUSED(callback);
        // TODO(PLUGIN-2): Implement dynamic toolbar registration
    }

    void showStatusMessage(const QString &message, int timeoutMs) override
    {
        if (m_mw->statusBar()) {
            m_mw->statusBar()->showMessage(message, timeoutMs);
        }
    }

    QVariant pluginSetting(const QString &key,
                           const QVariant &defaultValue) const override
    {
        QSettings settings;
        return settings.value("PluginSettings/" + key, defaultValue);
    }

    void setPluginSetting(const QString &key, const QVariant &value) override
    {
        QSettings settings;
        settings.setValue("PluginSettings/" + key, value);
    }

private:
    MainWindow *m_mw;
};

void MainWindow::initPluginSystem()
{
    m_editorAPI = new MainWindowEditorAPI(this);
    PluginManager::instance().setEditorAPI(m_editorAPI);
    PluginManager::instance().loadPlugins();
}

void MainWindow::showPluginDialog()
{
    if (!m_pluginDialog) {
        m_pluginDialog = new PluginDialog(this);
    }
    m_pluginDialog->exec();
}


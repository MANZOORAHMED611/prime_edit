#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QSplitter>
#include <QDockWidget>
#include <QMap>
#include "../core/remoteconnection.h"

class QPrinter;
class Editor;
class Document;
class TabWidget;
class LSPBridge;
class StatusBarWidget;
class SearchDialog;
class SearchResultsPanel;
class IncrementalSearchBar;
class PreferencesDialog;
class CommandPalette;
class TerminalWidget;
class NotificationBar;
class DocumentMapWidget;
class FunctionListPanel;
class LLMEvaluator;
class EvalResultWidget;
class IslamicBridge;
class SchemaValidator;
class EditorAPI;
class PluginDialog;
struct HadithValidation;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // Navigation
    int currentTabIndex() const;
    void setCurrentTabIndex(int index);
    Editor *currentEditor() const;

    // Tab access
    TabWidget *tabWidget() const { return m_tabWidget; }
    bool isSplit() const { return m_tabWidget2 != nullptr; }

    // Session
    QStringList openFilePaths() const;

public slots:
    // Encoding operations
    void convertToANSI();
    void convertToUTF8();
    void convertToUTF8BOM();
    void convertToUCS2BE();
    void convertToUCS2LE();

    // Line ending operations
    void convertToWindows();
    void convertToUnix();
    void convertToMac();

    // File operations
    void newFile();
    void openFile(const QString &filePath = QString());
    void saveFile();
    void saveFileAs();
    void saveAllFiles();
    bool closeFile(int index = -1);
    bool closeAllFiles();
    void printFile();
    void reloadFromDisk();

    // Indentation
    void increaseIndent();
    void decreaseIndent();

    // Navigation
    void goToLine(int line);

protected:
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onTabChanged(int index);
    void onTabCloseRequested(int index);
    void onDocumentModified(bool modified);
    void onCursorPositionChanged();

    // Edit menu
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();

    // Line operations
    void duplicateLine();
    void deleteLine();
    void moveLineUp();
    void moveLineDown();
    void toggleComment();
    void blockComment();
    void streamComment();
    void sortLinesAscending();
    void sortLinesDescending();
    void removeDuplicateLines();
    void removeEmptyLines();
    void removeEmptyLinesWithBlanks();
    void removeConsecutiveDuplicateLines();
    void joinLines();
    void splitLines();
    void trimTrailingWhitespace();
    void trimLeadingWhitespace();
    void trimLeadingAndTrailing();
    void eolToSpace();
    void removeUnnecessaryBlanks();
    void tabToSpace();
    void spaceToTabAll();
    void spaceToTabLeading();

    // Text case operations
    void toUpperCase();
    void toLowerCase();
    void toTitleCase();
    void toSentenceCase();
    void toInvertCase();
    void toRandomCase();

    // Search menu
    void find();
    void findNext();
    void findPrevious();
    void replace();
    void goToLineDialog();
    void showIncrementalSearch();
    void onSearchResultActivated(const QString &filePath, int line);
    void onFindAllInCurrent();
    void onFindAllInAllOpen();
    void onFindInFiles();
    void onCountRequested();
    void onIncrementalSearchChanged(const QString &text);

    // View menu
    void toggleWordWrap();
    void toggleLineNumbers();
    void toggleWhitespace();
    void toggleEndOfLine();
    void toggleIndentGuide();
    void toggleWrapSymbol();
    void toggleTerminal();
    void toggleFullScreen();
    void toggleDistractionFree();
    void syncVerticalScroll();
    void syncHorizontalScroll();
    void zoomIn();
    void zoomOut();
    void resetZoom();

    // Help
    void showAbout();
    void showPreferences();

    // Command palette
    void showCommandPalette();

    // Macro recording
    void startRecordingMacro();
    void stopRecordingMacro();
    void playbackMacro();
    void saveMacro();
    void loadMacro();
    void runMacroMultipleTimes();

    // Recent files
    void openRecentFile();
    void clearRecentFiles();
    void updateRecentFilesMenu();

    // Bookmark operations
    void toggleBookmark();
    void nextBookmark();
    void previousBookmark();
    void clearAllBookmarks();

    // Folding operations
    void foldAll();
    void unfoldAll();
    void toggleFold();
    void foldCurrentLevel();
    void unfoldCurrentLevel();

    // Document/Tab operations
    void closeOtherTabs();
    void closeTabsToRight();
    void closeTabsToLeft();
    void moveToOtherView();
    void cloneToOtherView();
    void previousTab();
    void nextTab();

    // Split view operations
    void splitVertical();
    void splitHorizontal();
    void closeSplit();
    void focusOtherSplit();

    // Column/Selection operations
    void columnMode();
    void columnEditor();
    void beginEndSelect();

    // Run menu
    void launchInTerminal();
    void openContainingFolder();

    // Window menu
    void updateWindowMenu();
    void switchToTab(int index);

    // View
    void toggleAlwaysOnTop();
    void showSummary();

    // LSP operations
    void gotoDefinition();
    void findReferences();
    void renameSymbol();

    // LLM evaluation
    void evaluateSelection();
    void onEvalResult(const QString &result);
    void onEvalAccepted(const QString &result);
    void onEvalRejected();
    void configureEndpoint();

    // Islamic knowledge integration
    void validateHadith();
    void onHadithValidated(const HadithValidation &result);
    void loadSchemaForDocument();
    void validateCurrentDocument();

    // Plugin operations
    void showPluginDialog();

    // Remote file operations
    void openRemoteFile();
    void onRemoteFileSelected(const RemoteConnection::ConnectionInfo &info,
                              const QString &remotePath);

    // Git operations
    void gitCommit();
    void gitBranchSwitch();
    void updateGitBranch();

private:
    void setupUi();
    void setupMenus();
    void setupToolBar();
    void setupStatusBar();
    void setupShortcuts();
    void updateWindowTitle();
    void updateStatusBar();
    void updateMenuState();
    void populateCommandPalette();

    Editor *createEditor(Document *document);
    int findEditorIndex(Editor *editor) const;
    int findEditorIndex(Document *document) const;

    void printDocument(QPrinter *printer, Editor *editor);
    TabWidget *activeTabWidget() const;
    TabWidget *inactiveTabWidget() const;
    void splitView(Qt::Orientation orientation);
    void unsplit();
    void connectTabWidget(TabWidget *tw);
    void onTab2CloseRequested(int index);
    void onTab2Changed(int index);
    void checkUnsplitNeeded();

    // Split view
    QSplitter *m_splitter = nullptr;
    TabWidget *m_tabWidget2 = nullptr;

    TabWidget *m_tabWidget = nullptr;
    StatusBarWidget *m_statusBar = nullptr;
    SearchDialog *m_searchDialog = nullptr;
    SearchResultsPanel *m_searchResultsPanel = nullptr;
    QDockWidget *m_searchResultsDock = nullptr;
    IncrementalSearchBar *m_incrementalSearchBar = nullptr;
    PreferencesDialog *m_preferencesDialog = nullptr;
    CommandPalette *m_commandPalette = nullptr;
    NotificationBar *m_notificationBar = nullptr;

    // Dock widgets
    QDockWidget *m_terminalDock = nullptr;
    TerminalWidget *m_terminal = nullptr;
    DocumentMapWidget *m_documentMap = nullptr;
    QDockWidget *m_documentMapDock = nullptr;
    FunctionListPanel *m_functionList = nullptr;
    QDockWidget *m_functionListDock = nullptr;

    // Menus
    QMenu *m_fileMenu = nullptr;
    QMenu *m_editMenu = nullptr;
    QMenu *m_searchMenu = nullptr;
    QMenu *m_viewMenu = nullptr;
    QMenu *m_toolsMenu = nullptr;
    QMenu *m_helpMenu = nullptr;
    QMenu *m_recentFilesMenu = nullptr;
    QMenu *m_encodingMenu = nullptr;
    QMenu *m_languageMenu = nullptr;
    QMenu *m_settingsMenu = nullptr;
    QMenu *m_macroMenu = nullptr;
    QMenu *m_runMenu = nullptr;
    QMenu *m_windowMenu = nullptr;

    // Toolbar
    QToolBar *m_mainToolBar = nullptr;

    // Actions for toolbar
    QAction *m_newAction = nullptr;
    QAction *m_openAction = nullptr;
    QAction *m_saveAction = nullptr;
    QAction *m_saveAllAction = nullptr;
    QAction *m_closeAction = nullptr;
    QAction *m_printAction = nullptr;
    QAction *m_cutAction = nullptr;
    QAction *m_copyAction = nullptr;
    QAction *m_pasteAction = nullptr;
    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;
    QAction *m_findAction = nullptr;
    QAction *m_replaceAction = nullptr;
    QAction *m_findNextAction = nullptr;
    QAction *m_findPrevAction = nullptr;
    QAction *m_zoomInAction = nullptr;
    QAction *m_zoomOutAction = nullptr;
    QAction *m_wordWrapAction = nullptr;
    QAction *m_showWhitespaceAction = nullptr;
    QAction *m_showEOLAction = nullptr;
    QAction *m_showIndentGuideAction = nullptr;
    QAction *m_toggleBookmarkAction = nullptr;
    QAction *m_nextBookmarkAction = nullptr;
    QAction *m_prevBookmarkAction = nullptr;
    QAction *m_clearBookmarksAction = nullptr;
    QAction *m_startRecordingAction = nullptr;
    QAction *m_stopRecordingAction = nullptr;
    QAction *m_playbackAction = nullptr;
    QAction *m_foldAllAction = nullptr;
    QAction *m_unfoldAllAction = nullptr;
    QAction *m_toggleCommentAction = nullptr;
    QAction *m_duplicateLineAction = nullptr;
    QAction *m_deleteLineAction = nullptr;

    // LSP
    LSPBridge *m_lspBridge = nullptr;

    // LLM evaluation
    LLMEvaluator *m_evaluator = nullptr;
    EvalResultWidget *m_evalResult = nullptr;
    QString m_evalOriginalText;

    // Islamic knowledge integration
    IslamicBridge *m_islamicBridge = nullptr;
    SchemaValidator *m_schemaValidator = nullptr;

    // Git integration
    QLabel *m_gitBranchLabel = nullptr;
    QMenu *m_gitMenu = nullptr;

    // Plugin system
    EditorAPI *m_editorAPI = nullptr;
    PluginDialog *m_pluginDialog = nullptr;
    void initPluginSystem();

    // Remote file tracking: local temp path -> (ConnectionInfo, remotePath)
    struct RemoteFileEntry {
        RemoteConnection::ConnectionInfo connInfo;
        QString remotePath;
    };
    QMap<QString, RemoteFileEntry> m_remoteFiles;
    void uploadRemoteFile(const QString &localPath);

    // State
    int m_untitledCounter = 0;
    bool m_closingAll = false;
    bool m_distractionFree = false;
};

#endif // MAINWINDOW_H

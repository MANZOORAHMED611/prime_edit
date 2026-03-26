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

class Editor;
class Document;
class TabWidget;
class StatusBarWidget;
class SearchDialog;
class SearchResultsPanel;
class IncrementalSearchBar;
class PreferencesDialog;
class CommandPalette;
class TerminalWidget;

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

    TabWidget *m_tabWidget;
    StatusBarWidget *m_statusBar;
    SearchDialog *m_searchDialog = nullptr;
    SearchResultsPanel *m_searchResultsPanel = nullptr;
    QDockWidget *m_searchResultsDock = nullptr;
    IncrementalSearchBar *m_incrementalSearchBar = nullptr;
    PreferencesDialog *m_preferencesDialog = nullptr;
    CommandPalette *m_commandPalette = nullptr;

    // Dock widgets
    QDockWidget *m_terminalDock = nullptr;
    TerminalWidget *m_terminal = nullptr;

    // Menus
    QMenu *m_fileMenu;
    QMenu *m_editMenu;
    QMenu *m_searchMenu;
    QMenu *m_viewMenu;
    QMenu *m_toolsMenu;
    QMenu *m_helpMenu;
    QMenu *m_recentFilesMenu;
    QMenu *m_encodingMenu;
    QMenu *m_languageMenu;
    QMenu *m_settingsMenu;
    QMenu *m_macroMenu;
    QMenu *m_runMenu;
    QMenu *m_windowMenu;

    // Toolbar
    QToolBar *m_mainToolBar;

    // Actions for toolbar
    QAction *m_newAction;
    QAction *m_openAction;
    QAction *m_saveAction;
    QAction *m_saveAllAction;
    QAction *m_closeAction;
    QAction *m_printAction;
    QAction *m_cutAction;
    QAction *m_copyAction;
    QAction *m_pasteAction;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_findAction;
    QAction *m_replaceAction;
    QAction *m_findNextAction;
    QAction *m_findPrevAction;
    QAction *m_zoomInAction;
    QAction *m_zoomOutAction;
    QAction *m_wordWrapAction;
    QAction *m_showWhitespaceAction;
    QAction *m_showEOLAction;
    QAction *m_showIndentGuideAction;
    QAction *m_toggleBookmarkAction;
    QAction *m_nextBookmarkAction;
    QAction *m_prevBookmarkAction;
    QAction *m_clearBookmarksAction;
    QAction *m_startRecordingAction;
    QAction *m_stopRecordingAction;
    QAction *m_playbackAction;
    QAction *m_foldAllAction;
    QAction *m_unfoldAllAction;
    QAction *m_toggleCommentAction;
    QAction *m_duplicateLineAction;
    QAction *m_deleteLineAction;

    // Status bar labels
    QLabel *m_positionLabel;
    QLabel *m_encodingLabel;
    QLabel *m_lineEndingLabel;
    QLabel *m_languageLabel;

    // State
    int m_untitledCounter = 0;
    bool m_closingAll = false;
};

#endif // MAINWINDOW_H

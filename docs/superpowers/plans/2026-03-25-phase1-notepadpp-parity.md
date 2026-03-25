# Phase 1: Notepad++ Parity Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Transform Olive Notepad to match Notepad++ visual identity, editor core capabilities, and search system.

**Architecture:** Existing C++17/Qt application with PieceTable buffer, Document/Editor separation. Changes are additive — modify existing files to match N++ UI, harden the editor core with new rendering features, and replace the search system entirely. No new dependencies beyond Qt.

**Tech Stack:** C++17, Qt5/Qt6 (dual support via CMakeLists.txt), CMake 3.16+

**Spec:** `docs/superpowers/specs/2026-03-25-phase1-notepadpp-parity-design.md`

**Build command:** `cd build && cmake .. && make -j$(nproc)`

**Run command:** `cd build && ./"Olive Notepad"`

---

## Pre-Flight: Verify Build

Before any changes, confirm the project compiles and runs.

- [ ] **Step 1: Clean build from scratch**

```bash
cd /home/abusulaiman/dev/repository/olive_notepad
rm -rf build && mkdir build && cd build
cmake ..
make -j$(nproc)
```

Expected: Compiles with 0 errors. Warnings are acceptable.

- [ ] **Step 2: Run the binary**

```bash
cd /home/abusulaiman/dev/repository/olive_notepad/build
./"Olive Notepad" &
```

Expected: Window opens with olive-themed editor. Close it after visual confirmation.

- [ ] **Step 3: Remove dead files**

Delete stale backup files that should not be in the repo:

```bash
rm src/ui/toolbarmanager_old.cpp src/ui/toolbarmanager.cpp.backup
```

- [ ] **Step 4: Commit baseline**

```bash
git add -A && git commit -m "chore: remove stale toolbarmanager backup files"
```

---

## Task 1: Theme System — New Fields + Notepad++ Theme

**Files:**
- Modify: `src/ui/theme.h` — add 7 new QColor fields to Theme struct
- Modify: `src/ui/theme.cpp` — add JSON serialization for new fields, add `notepadpp()` static method, update `toStyleSheet()`
- Create: `resources/themes/notepadpp.json` — Notepad++ classic color palette
- Modify: `resources/resources.qrc` — add notepadpp.json entry
- Modify: `src/main.cpp` — change default theme from "Olive" to "Notepad++"

### Steps

- [ ] **Step 1: Add new color fields to Theme struct**

In `src/ui/theme.h`, add after the existing `borderColor` field (before diagnostic colors):

```cpp
// Gutter colors
QColor foldMarginBackground;
QColor bookmarkMarginBackground;

// Editor feature colors
QColor indentGuideColor;
QColor whitespaceColor;
QColor markHighlightColor;
QColor bracketMatchBackground;
QColor bracketErrorBackground;
```

- [ ] **Step 2: Add JSON serialization for new fields**

In `src/ui/theme.cpp`, in `toJson()` add the 7 new fields after the accent colors block. In `fromJson()` add the corresponding deserialization. Use same `colorToString`/`stringToColor` pattern as existing fields.

- [ ] **Step 3: Add `notepadpp()` static method**

In `src/ui/theme.h`, add declaration: `static Theme notepadpp();`

In `src/ui/theme.cpp`, implement `Theme::notepadpp()` returning the full Notepad++ color palette from the spec:
- Background: `#ffffff`, foreground: `#000000`
- Toolbar: `#f0f0f0`, status bar: `#f0f0f0`/`#000000`
- Tabs: active `#ffffff`, inactive `#e8e8e8`
- Line numbers: `#808080` on `#e4e4e4`, current line: `#e8e8ff`
- Selection: `#0078d7`, fold margin: `#f0f0f0`, bookmark margin: `#e4e4e4`
- Keywords: `#0000ff`, strings: `#808080`, comments: `#008000`, numbers: `#ff8000`
- New fields: indentGuide `#d0d0d0`, whitespace `#c0c0c0`, markHighlight `#ffff00`, bracketMatch `#90ee90`, bracketError `#ff6666`

- [ ] **Step 4: Update built-in themes loading**

In `src/ui/theme.cpp`, find `loadBuiltInThemes()` and add `m_themes["Notepad++"] = Theme::notepadpp();`

- [ ] **Step 5: Update default theme values for all existing themes**

Ensure `defaultLight()`, `defaultDark()`, `olive()`, and all other built-in theme methods set sensible defaults for the 7 new fields (so they don't render as black/invalid).

- [ ] **Step 6: Create notepadpp.json theme file**

Create `resources/themes/notepadpp.json` with the full color palette in the same JSON format as `olive.json`. Use the same key naming convention as `olive.json` (`colors` and `tokenColors` nested objects).

- [ ] **Step 7: Add to resources.qrc**

In `resources/resources.qrc`, add `<file>themes/notepadpp.json</file>` in the themes section.

- [ ] **Step 8: Change default theme**

In `src/main.cpp`, change `ThemeManager::instance().applyTheme("Olive");` to `ThemeManager::instance().applyTheme("Notepad++");`

- [ ] **Step 9: Build and verify**

```bash
cd build && cmake .. && make -j$(nproc) && ./"Olive Notepad" &
```

Expected: Application opens with white editor background, light grey toolbars, Notepad++-style colors. Close and verify.

- [ ] **Step 10: Commit**

```bash
git add -A && git commit -m "feat: add Notepad++ theme with 7 new Theme struct color fields"
```

---

## Task 2: Single-Row Toolbar

**Files:**
- Modify: `src/ui/toolbarmanager.h` — single `QToolBar*` member, remove per-toolbar accessors
- Modify: `src/ui/toolbarmanager.cpp` — single-row dense toolbar layout
- Modify: `src/ui/mainwindow.h` — replace six `QToolBar*` members with single `QToolBar *m_mainToolBar`
- Modify: `src/ui/mainwindow.cpp` — update `setupToolBar()` to use single toolbar reference

### Steps

- [ ] **Step 1: Simplify ToolbarManager header**

In `src/ui/toolbarmanager.h`:
- Replace six `QToolBar*` members with single `QToolBar *m_toolbar`
- Remove `fileToolbar()`, `editToolbar()`, `searchToolbar()`, `viewToolbar()`, `formatToolbar()`, `macroToolbar()` accessors
- Add single `QToolBar* toolbar() const { return m_toolbar; }`
- Remove six `createXxxToolbar()` private methods, replace with single `void populateToolbar()`

- [ ] **Step 2: Rewrite ToolbarManager implementation**

In `src/ui/toolbarmanager.cpp`, rewrite `createToolbars()`:
- Create single toolbar: `m_toolbar = m_mainWindow->addToolBar(tr("Main"));`
- Set icon size 16x16: `m_toolbar->setIconSize(QSize(16, 16));`
- Set not movable: `m_toolbar->setMovable(false);`
- Remove ALL `addToolBarBreak()` calls
- Add actions in single row following this grouping (with separators between groups):

Group 1 — File: New, Open, Save, SaveAll, Close, CloseAll
Group 2 — Edit: Cut, Copy, Paste, Undo, Redo
Group 3 — Search: Find, Replace
Group 4 — View: ZoomIn, ZoomOut, WordWrap, ShowWhitespace, ShowEOL, IndentGuide
Group 5 — Macro: Record, Stop, Play
Group 6 — Format: Comment, Indent, Outdent

Remove all non-essential toolbar items (closeOthers, closeRight, closeLeft, joinLines, splitLines, blockComment, selectAll, duplicateLine, deleteLine, moveUp, moveDown, uppercase, lowercase, titleCase, invertCase, trim*, tabToSpace, spaceToTab, removeEmpty, removeDups, sort*, fullScreen, distractionFree, foldAll, unfoldAll, toggleFold, gotoLine, prevTab, nextTab, bookmark buttons, saveMacro, loadMacro). These remain accessible via menus only.

Keep using existing SVG icons from `:/icons/toolbar/`. Keep using `createAction()` helper and `SLOT()` connections to MainWindow.

- [ ] **Step 3: Update MainWindow header**

In `src/ui/mainwindow.h`:
- Remove: `QToolBar *m_fileToolBar;` through `QToolBar *m_macroToolBar;` (6 members)
- Add: `QToolBar *m_mainToolBar;`

- [ ] **Step 4: Update MainWindow setupToolBar()**

In `src/ui/mainwindow.cpp`, update `setupToolBar()`:
```cpp
void MainWindow::setupToolBar()
{
    ToolbarManager *toolbarMgr = new ToolbarManager(this, this);
    toolbarMgr->createToolbars();
    m_mainToolBar = toolbarMgr->toolbar();
}
```

- [ ] **Step 5: Build and verify**

```bash
cd build && cmake .. && make -j$(nproc) && ./"Olive Notepad" &
```

Expected: Single dense toolbar row with 16x16 icons. No toolbar breaks. All icons visible.

- [ ] **Step 6: Commit**

```bash
git add -A && git commit -m "feat: condense toolbar to single Notepad++ style row with 16x16 icons"
```

---

## Task 3: Enhanced Tab Bar

**Files:**
- Modify: `src/ui/tabwidget.h` — add signals, file-type icon support
- Modify: `src/ui/tabwidget.cpp` — extended context menu, file-type icons, close-to-left

### Steps

- [ ] **Step 1: Add file-type icon mapping to TabBar**

In `src/ui/tabwidget.h`, add to `TabBar`:
```cpp
public:
    static QIcon iconForFile(const QString &filePath);
signals:
    void closeToLeftRequested(int index);
```

In `src/ui/tabwidget.cpp`, implement `TabBar::iconForFile()`:
- Map common extensions to QIcon created from colored pixmaps:
  - `.cpp`, `.h`, `.c` → blue document icon
  - `.py` → green document icon
  - `.js`, `.ts` → yellow document icon
  - `.html`, `.css` → orange document icon
  - `.json`, `.xml`, `.yaml`, `.yml` → purple document icon
  - `.md`, `.txt` → grey document icon
  - default → generic document icon
- Create icons programmatically using QPainter on a 16x16 QPixmap (colored rectangle with file extension text).

- [ ] **Step 2: Extend context menu**

In `TabBar::contextMenuEvent()`, add after existing items:
- "Close to the Left" action → emit `closeToLeftRequested(index)`
- Separator
- "Save" action → emit `tabCloseRequested(index)` (will need a new signal or direct call)
- "Copy Full Path" action → copy file path to clipboard using `QApplication::clipboard()`
- "Open Containing Folder" action → use `QDesktopServices::openUrl(QUrl::fromLocalFile(dirPath))`

- [ ] **Step 3: Add closeToLeft handler to TabWidget**

In `src/ui/tabwidget.h` add slot: `void onCloseToLeftRequested(int index);`

In `src/ui/tabwidget.cpp`:
```cpp
void TabWidget::onCloseToLeftRequested(int index)
{
    for (int i = index - 1; i >= 0; --i) {
        emit tabCloseRequested(i);
    }
}
```

Connect in TabWidget constructor: `connect(m_tabBar, &TabBar::closeToLeftRequested, this, &TabWidget::onCloseToLeftRequested);`

- [ ] **Step 4: Set tab scroll mode**

In `TabBar` constructor, add:
```cpp
setUsesScrollButtons(true);  // Scroll arrows when tabs overflow
```

- [ ] **Step 5: Integrate file-type icons into MainWindow**

In `src/ui/mainwindow.cpp`, update `openFile()` and `newFile()` to call `m_tabWidget->setTabIcon(index, TabBar::iconForFile(filePath))` after adding the tab. Update `onDocumentModified()` to swap to a red-tinted icon when modified.

- [ ] **Step 6: Build and verify**

```bash
cd build && cmake .. && make -j$(nproc) && ./"Olive Notepad" &
```

Expected: Tabs show colored file-type icons. Right-click shows extended menu. Tab overflow shows scroll arrows.

- [ ] **Step 7: Commit**

```bash
git add -A && git commit -m "feat: enhanced tab bar with file-type icons, extended context menu, scroll arrows"
```

---

## Task 4: Enhanced Status Bar

**Files:**
- Modify: `src/ui/statusbar.h` — new sections, clickable indicators
- Modify: `src/ui/statusbar.cpp` — full Notepad++ status bar layout

### Steps

- [ ] **Step 1: Redesign StatusBarWidget header**

In `src/ui/statusbar.h`, replace the current QLabel members with:

```cpp
private:
    QLabel *m_docTypeLabel;      // Document type + icon
    QLabel *m_lengthLabel;       // Length: X  Lines: Y
    QLabel *m_positionLabel;     // Ln: X  Col: Y  Sel: Z  Pos: W
    QPushButton *m_eolButton;    // CR LF / LF / CR — clickable
    QPushButton *m_encodingButton; // UTF-8 / ANSI — clickable
    QLabel *m_insertModeLabel;   // INS / OVR

    QMenu *m_eolMenu;
    QMenu *m_encodingMenu;
```

Add new public slots:
```cpp
void setDocType(const QString &type);
void setLength(qint64 length, int lines);
void setPosition(int line, int column, int selCount, qint64 pos);
void setInsertMode(bool insert);
```

- [ ] **Step 2: Implement new status bar layout**

In `src/ui/statusbar.cpp`, rewrite constructor:
- Create all widgets with sunken frame style: `label->setFrameStyle(QFrame::Panel | QFrame::Sunken);`
- Add them left-to-right using `addPermanentWidget()` for right-aligned sections
- Create EOL popup menu with actions: "Windows (CR LF)", "Unix (LF)", "Mac (CR)" — connect to MainWindow conversion slots
- Create encoding popup menu with actions: "UTF-8", "UTF-8-BOM", "ANSI", "UCS-2 BE", "UCS-2 LE" — connect to MainWindow conversion slots
- Style: light grey background matching Notepad++ theme, small padding

- [ ] **Step 3: Update MainWindow to feed new status bar**

In `src/ui/mainwindow.cpp`, update `updateStatusBar()` and `onCursorPositionChanged()` to call the new status bar methods with selection count and absolute position. Add Insert key handling to toggle INS/OVR mode.

- [ ] **Step 4: Build and verify**

```bash
cd build && cmake .. && make -j$(nproc) && ./"Olive Notepad" &
```

Expected: Status bar shows all N++ sections. Clicking encoding/EOL shows popup menus.

- [ ] **Step 5: Commit**

```bash
git add -A && git commit -m "feat: Notepad++ style status bar with clickable encoding/EOL indicators"
```

---

## Task 5: Menu Restructure

**Files:**
- Modify: `src/ui/mainwindow.h` — new menu pointers for Encoding, Language, Settings, Run, Window
- Modify: `src/ui/mainwindow.cpp` — rewrite `setupMenus()` to match N++ menu structure

### Steps

- [ ] **Step 1: Add new menu member pointers**

In `src/ui/mainwindow.h`, add:
```cpp
QMenu *m_encodingMenu;
QMenu *m_languageMenu;
QMenu *m_settingsMenu;
QMenu *m_macroMenu;
QMenu *m_runMenu;
QMenu *m_windowMenu;
```

- [ ] **Step 2: Rewrite setupMenus()**

In `src/ui/mainwindow.cpp`, restructure `setupMenus()` to create menus in Notepad++ order:

```
File | Edit | Search | View | Encoding | Language | Settings | Tools | Macro | Run | Window | ?
```

**File menu** — keep existing items, add: "Open Containing Folder", "Rename" (stub)

**Edit menu** — keep existing items, add: "Column Editor..." (Alt+C) connected to new slot `showColumnEditor()`

**Search menu** — keep existing items, update shortcuts to match N++ (Ctrl+F find, Ctrl+H replace, Ctrl+Shift+F find in files, Ctrl+I incremental search)

**View menu** — keep existing items, add: "Always on Top" (checkable), "Summary" (shows line/word/char count dialog)

**Encoding menu** — new, with actions: "Encode in UTF-8", "Encode in UTF-8-BOM", "Encode in ANSI", "Encode in UCS-2 BE BOM", "Encode in UCS-2 LE BOM". Use `QActionGroup` for mutual exclusivity. Convert actions below separator.

**Language menu** — new, with "Auto-detect" at top, then categorized language list from `LanguageManager`. Connect to `Editor::setLanguage()`.

**Settings menu** — new, with "Preferences..." (existing), "Style Configurator..." (stub), "Shortcut Mapper..." (stub)

**Tools menu** — keep existing minus macro items (moved to Macro menu)

**Macro menu** — move macro items from Tools: Start Recording, Stop Recording, Playback, separator, Save Macro, Load Macro

**Run menu** — new, with: "Launch in Terminal" (opens terminal at file's directory), "Open Containing Folder" (QDesktopServices)

**Window menu** — new, dynamically populated with list of open documents. Connect to tab switching.

**? (Help) menu** — rename from "Help" to "?", keep About items

- [ ] **Step 3: Add new slot stubs**

In `src/ui/mainwindow.h`, add slot declarations for any new actions that don't have implementations yet. In `src/ui/mainwindow_extended.cpp`, add minimal stub implementations (show "Not yet implemented" QMessageBox or no-op).

- [ ] **Step 4: Add Window menu dynamic population**

Add a slot `updateWindowMenu()` connected to `m_tabWidget`'s `currentChanged` signal. It clears and rebuilds the Window menu with one action per open tab, checkmarking the current one. Connect each action to switch to that tab index.

- [ ] **Step 5: Build and verify**

```bash
cd build && cmake .. && make -j$(nproc) && ./"Olive Notepad" &
```

Expected: All 12 menus visible in menu bar. Encoding menu shows encoding options. Language menu lists languages. Window menu shows open documents.

- [ ] **Step 6: Commit**

```bash
git add -A && git commit -m "feat: restructure menus to match Notepad++ layout (12 menus)"
```

---

## Task 6: Three-Column Gutter + Bookmarks

**Files:**
- Modify: `src/ui/linenumberarea.h` — add gutter section constants
- Modify: `src/ui/linenumberarea.cpp` — three-section paint event, click handling
- Modify: `src/ui/editor.h` — bookmark data model, gutter width calculation, margin constants
- Modify: `src/ui/editor.cpp` — bookmark management, gutter width, paint delegation

### Steps

- [ ] **Step 1: Define gutter constants in editor.h**

In `src/ui/editor.h`, add:
```cpp
static constexpr int BOOKMARK_MARGIN_WIDTH = 16;
static constexpr int FOLD_MARGIN_WIDTH = 16;
```

Add bookmark members:
```cpp
QSet<int> m_bookmarks;
bool m_bookmarkMarginVisible = true;
bool m_foldMarginVisible = true;
```

Add public methods:
```cpp
void toggleBookmark(int line);
void nextBookmark();
void previousBookmark();
void clearBookmarks();
QSet<int> bookmarks() const { return m_bookmarks; }
int bookmarkMarginWidth() const;
int foldMarginWidth() const;
int totalGutterWidth() const;
```

- [ ] **Step 2: Update lineNumberAreaWidth()**

In `src/ui/editor.cpp`, modify `lineNumberAreaWidth()` to return the sum of all three margins:
```cpp
int Editor::lineNumberAreaWidth() const
{
    int width = 0;
    if (m_bookmarkMarginVisible) width += BOOKMARK_MARGIN_WIDTH;
    if (m_lineNumbersVisible) {
        int digits = 1;
        int max = qMax(1, blockCount());
        while (max >= 10) { max /= 10; ++digits; }
        width += 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    }
    if (m_foldMarginVisible) width += FOLD_MARGIN_WIDTH;
    return width;
}
```

- [ ] **Step 3: Rewrite lineNumberAreaPaintEvent()**

In `src/ui/editor.cpp`, rewrite `lineNumberAreaPaintEvent()` to paint three sections:

1. **Bookmark margin** (left 16px): light background. For each visible block, if block number is in `m_bookmarks`, draw a filled blue circle (radius 5, centered in the margin).

2. **Line numbers** (middle, variable width): existing line number painting logic, but offset by `bookmarkMarginWidth()` from left edge.

3. **Fold margin** (right 16px): light background. For now, paint only the background — fold indicators will be added in Task 8 (code folding). Leave a `// TODO: fold indicators` placeholder.

- [ ] **Step 4: Handle bookmark margin clicks**

In `src/ui/linenumberarea.h`, add:
```cpp
protected:
    void mousePressEvent(QMouseEvent *event) override;
```

In `src/ui/linenumberarea.cpp`, implement click handling:
- If click x-position is within bookmark margin bounds (0 to BOOKMARK_MARGIN_WIDTH), determine the line number from y-position and call `m_editor->toggleBookmark(line)`.
- If click x-position is within fold margin bounds, delegate to fold toggle (stub for now).

- [ ] **Step 5: Implement bookmark operations in Editor**

In `src/ui/editor.cpp`:

```cpp
void Editor::toggleBookmark(int line)
{
    if (m_bookmarks.contains(line))
        m_bookmarks.remove(line);
    else
        m_bookmarks.insert(line);
    m_lineNumberArea->update();
}

void Editor::nextBookmark()
{
    if (m_bookmarks.isEmpty()) return;
    int current = currentLine();
    auto it = std::upper_bound(m_bookmarks.begin(), m_bookmarks.end(), current);
    if (it == m_bookmarks.end()) it = m_bookmarks.begin();
    goToLine(*it);
}

void Editor::previousBookmark()
{
    if (m_bookmarks.isEmpty()) return;
    int current = currentLine();
    QList<int> sorted = m_bookmarks.values();
    std::sort(sorted.begin(), sorted.end());
    auto it = std::lower_bound(sorted.begin(), sorted.end(), current);
    if (it == sorted.begin()) it = sorted.end();
    --it;
    goToLine(*it);
}

void Editor::clearBookmarks()
{
    m_bookmarks.clear();
    m_lineNumberArea->update();
}
```

- [ ] **Step 6: Connect MainWindow bookmark slots**

In `src/ui/mainwindow.cpp`, ensure `toggleBookmark()`, `nextBookmark()`, `previousBookmark()`, `clearAllBookmarks()` slots delegate to `currentEditor()`.

- [ ] **Step 7: Build and verify**

```bash
cd build && cmake .. && make -j$(nproc) && ./"Olive Notepad" &
```

Expected: Three-column gutter visible. Click bookmark margin → blue circle appears. F2 navigates between bookmarks. Fold margin shows as empty grey column.

- [ ] **Step 8: Commit**

```bash
git add -A && git commit -m "feat: three-column gutter with bookmark margin and navigation"
```

---

## Task 7: Bracket Matching

**Files:**
- Modify: `src/ui/editor.h` — bracket matching members and methods
- Modify: `src/ui/editor.cpp` — bracket matching logic and highlighting

### Steps

- [ ] **Step 1: Add bracket matching to Editor**

In `src/ui/editor.h`, add:
```cpp
private:
    void matchBrackets();
    int findMatchingBracket(int position, QChar open, QChar close, bool forward) const;
    QList<QTextEdit::ExtraSelection> m_bracketSelections;
public slots:
    void jumpToMatchingBracket();
```

- [ ] **Step 2: Implement bracket matching logic**

In `src/ui/editor.cpp`:

`matchBrackets()` — called from `highlightCurrentLine()` (which is connected to `cursorPositionChanged`):
- Get character at cursor position and position-1
- If either is a bracket `()[]{}`, find its match using stack-based scanning
- Create two `ExtraSelection` entries with `bracketMatchBackground` from theme (or `bracketErrorBackground` if no match found)
- Store in `m_bracketSelections`
- Call `setExtraSelections()` combining bracket selections with current line highlight

`findMatchingBracket()` — stack-based scanner:
- Start from position, scan forward (for open brackets) or backward (for close brackets)
- Track nesting depth with a counter
- Return position of matching bracket, or -1 if not found
- Skip brackets inside strings and comments by checking `QTextBlock::userState()` from the syntax highlighter

- [ ] **Step 3: Implement jump to matching bracket**

```cpp
void Editor::jumpToMatchingBracket()
{
    // Get bracket at cursor, find match, move cursor there
    int pos = textCursor().position();
    QString text = toPlainText();
    if (pos >= text.length()) return;
    QChar ch = text.at(pos);
    // ... (matching logic, move cursor to result)
}
```

Connect Ctrl+B shortcut in `setupShortcuts()`.

- [ ] **Step 4: Build and verify**

Open a source file, place cursor next to a bracket. Both brackets should highlight green. Unmatched brackets highlight red. Ctrl+B jumps to match.

- [ ] **Step 5: Commit**

```bash
git add -A && git commit -m "feat: bracket matching with highlight and Ctrl+B jump"
```

---

## Task 8: Indent Guides + Whitespace Rendering

**Files:**
- Modify: `src/ui/editor.h` — whitespace/EOL visibility state, indent guide flag
- Modify: `src/ui/editor.cpp` — custom painting for indent guides, whitespace, and EOL symbols

### Steps

- [ ] **Step 1: Add visibility state to Editor**

In `src/ui/editor.h`, add members:
```cpp
bool m_showWhitespace = false;
bool m_showEOL = false;
bool m_showIndentGuide = false;
```

Add public setters:
```cpp
void setShowWhitespace(bool show);
void setShowEOL(bool show);
void setShowIndentGuide(bool show);
```

- [ ] **Step 2: Install viewport event filter**

In Editor constructor:
```cpp
viewport()->installEventFilter(this);
```

Add to `src/ui/editor.h`:
```cpp
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
```

In `src/ui/editor.cpp`, implement `eventFilter()`:
```cpp
bool Editor::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == viewport() && event->type() == QEvent::Paint) {
        // Let base class paint first
        QPlainTextEdit::paintEvent(static_cast<QPaintEvent*>(event));
        // Then overlay our custom rendering
        paintWhitespace();
        paintIndentGuides();
        return true;  // We handled it
    }
    return QPlainTextEdit::eventFilter(obj, event);
}
```

Note: Remove the base class `paintEvent` call from the normal path since we're intercepting via event filter now.

- [ ] **Step 3: Implement whitespace rendering**

Add private method `void paintWhitespace()` in `src/ui/editor.cpp`:
- Get visible block range from `firstVisibleBlock()`
- For each visible block, iterate characters:
  - Spaces: draw `·` (centered dot) at the character position using `QPainter` on viewport
  - Tabs: draw `→` at the tab start position
- Only paint if `m_showWhitespace` is true

Add private method `void paintEOL()`:
- For each visible line, draw `¶` or `LF`/`CRLF` symbol at end of line
- Only paint if `m_showEOL` is true

Use theme's `whitespaceColor` for the paint color.

- [ ] **Step 4: Implement indent guide rendering**

Add private method `void paintIndentGuides()` in `src/ui/editor.cpp`:
- Only paint if `m_showIndentGuide` is true
- For each visible block, determine its indent level (count leading tabs/spaces)
- Draw vertical dotted line at each tab-stop column using `QPen(Qt::DotLine)` with theme's `indentGuideColor`
- Lines extend from block top to block bottom

- [ ] **Step 5: Connect MainWindow toggle slots**

In `src/ui/mainwindow.cpp`, update `toggleWhitespace()`, `toggleEndOfLine()`, `toggleIndentGuide()` to call the corresponding Editor setters and trigger viewport update.

- [ ] **Step 6: Build and verify**

Open a code file. Toggle whitespace on → spaces show as dots, tabs as arrows. Toggle EOL → line ending symbols appear. Toggle indent guide → vertical dotted lines at indent levels.

- [ ] **Step 7: Commit**

```bash
git add -A && git commit -m "feat: whitespace rendering, EOL visualization, and indent guides"
```

---

## Task 9: Code Folding

**Files:**
- Modify: `src/ui/editor.h` — fold state management, fold detection
- Modify: `src/ui/editor.cpp` — fold/unfold logic using block-caching approach
- Modify: `src/ui/linenumberarea.cpp` — fold margin indicators

### Steps

- [ ] **Step 1: Add fold state to Editor**

In `src/ui/editor.h`, add:
```cpp
private:
    QMap<int, QStringList> m_foldedRegions;  // header block number → cached folded lines
    QSet<int> m_foldableLines;               // lines that can be folded (have fold regions)

    void detectFoldableRegions();
    QPair<int, int> findFoldRegion(int blockNumber) const;  // returns {startLine, endLine}
    bool isFoldableLine(int blockNumber) const;
    bool isFoldedLine(int blockNumber) const;
public:
    void foldAt(int blockNumber);
    void unfoldAt(int blockNumber);
    void toggleFoldAt(int blockNumber);
    void foldAll();
    void unfoldAll();
    QMap<int, QStringList> foldedRegions() const { return m_foldedRegions; }
```

- [ ] **Step 2: Implement fold region detection**

In `src/ui/editor.cpp`:

`detectFoldableRegions()` — called after text changes (debounced):
- Scan all blocks for brace-based fold points: line contains `{` and nesting level increases
- For indent-based languages (Python, YAML — check language): fold point where indent level increases
- Store foldable line numbers in `m_foldableLines`

`findFoldRegion(blockNumber)` — given a foldable line, find the end of its fold region:
- Brace-based: count matching braces until balanced
- Indent-based: scan forward until indent level returns to or below the fold line's level

- [ ] **Step 3: Implement fold/unfold using block caching**

```cpp
void Editor::foldAt(int blockNumber)
{
    auto region = findFoldRegion(blockNumber);
    if (region.first < 0) return;

    QTextCursor cursor(document()->findBlockByNumber(region.first));
    QStringList cachedLines;

    // Cache the lines that will be removed
    for (int i = region.first + 1; i <= region.second; ++i) {
        QTextBlock block = document()->findBlockByNumber(region.first + 1);
        if (!block.isValid()) break;
        cachedLines.append(block.text());
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        if (i < region.second)
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }

    m_foldedRegions[blockNumber] = cachedLines;

    // Append [...] indicator to the fold header line
    QTextBlock headerBlock = document()->findBlockByNumber(blockNumber);
    cursor = QTextCursor(headerBlock);
    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.insertText(" [...]");

    m_lineNumberArea->update();
}
```

`unfoldAt(blockNumber)` — reverse: remove `[...]` suffix, re-insert cached lines after header block.

- [ ] **Step 4: Paint fold indicators in gutter**

In `src/ui/linenumberarea.cpp`, in the fold margin paint section:
- For each visible block:
  - If `m_editor->isFoldableLine(blockNum)` and not folded: draw `[-]` box
  - If folded: draw `[+]` box
  - If inside a fold region (between fold start and fold end): draw vertical line connecting fold indicators

Use QPainter to draw small 9x9 boxes with +/- signs, matching Notepad++ fold indicator style.

- [ ] **Step 5: Handle fold margin clicks**

In `src/ui/linenumberarea.cpp` `mousePressEvent()`:
- If click is in fold margin area, determine block number and call `m_editor->toggleFoldAt(blockNumber)`.

- [ ] **Step 6: Connect MainWindow fold slots**

Ensure `foldAll()`, `unfoldAll()`, `toggleFold()` delegate to current editor.

- [ ] **Step 7: Build and verify**

Open a C/C++ file with functions. Click fold indicators — functions collapse showing `[...]`. Click again — they expand. Fold All/Unfold All work.

- [ ] **Step 8: Commit**

```bash
git add -A && git commit -m "feat: code folding with block-caching, fold margin indicators"
```

---

## Task 10: Column Selection + Column Editor

**Files:**
- Modify: `src/ui/editor.h` — column selection state and methods
- Modify: `src/ui/editor.cpp` — Alt+drag column selection, viewport overlay painting
- Create: `src/ui/columneditor.h` — column editor dialog
- Create: `src/ui/columneditor.cpp` — column editor implementation
- Modify: `CMakeLists.txt` — add columneditor source files

### Steps

- [ ] **Step 1: Add column selection state to Editor**

In `src/ui/editor.h`:
```cpp
private:
    struct ColumnSelection {
        int startLine = -1, startCol = -1;
        int endLine = -1, endCol = -1;
        bool active = false;
    };
    ColumnSelection m_columnSelection;

    void paintColumnSelection(QPainter &painter);
    void insertTextAtColumn(const QString &text);
public:
    bool isColumnSelectionActive() const { return m_columnSelection.active; }
    void clearColumnSelection();
```

- [ ] **Step 2: Implement Alt+drag column selection**

In `src/ui/editor.cpp`:

Override `mousePressEvent()` — if Alt is held, start column selection mode. Record start line/column from mouse position.

Override `mouseMoveEvent()` — if column selection active, update end line/column from current mouse position. Trigger viewport update.

In `eventFilter()` paint handler, add `paintColumnSelection()` call:
- Draw a semi-transparent blue rectangle covering the column selection area
- Calculate pixel positions from line/column using `blockBoundingGeometry()` and `fontMetrics().horizontalAdvance()`

- [ ] **Step 3: Implement column typing**

In `keyPressEvent()` — if column selection is active and a printable character is typed:
- Insert that character at the column position on every line in the selection range
- Use `beginEditBlock()`/`endEditBlock()` for single undo

- [ ] **Step 4: Create ColumnEditor dialog**

Create `src/ui/columneditor.h` and `src/ui/columneditor.cpp`:

```
Column Editor Dialog:
  [Text to Insert: __________]
  -- or --
  [Number to Insert]
  [Initial Number: ___]
  [Increase by: ___]
  [Leading Zeros: checkbox]
  [OK] [Cancel]
```

A QDialog with two modes: text insertion or number sequence. On OK, inserts text/numbers at the column selection position across all selected lines.

- [ ] **Step 5: Update CMakeLists.txt**

Add `src/ui/columneditor.cpp` to `UI_SOURCES` and `src/ui/columneditor.h` to `HEADERS`.

- [ ] **Step 6: Connect MainWindow**

Add `showColumnEditor()` slot to MainWindow, connected to Edit > Column Editor menu item. Opens the ColumnEditor dialog.

- [ ] **Step 7: Build and verify**

Alt+drag in editor creates blue rectangle column selection. Type to insert at each line. Edit > Column Editor dialog works for text and number insertion.

- [ ] **Step 8: Commit**

```bash
git add -A && git commit -m "feat: column/block selection with Alt+drag and Column Editor dialog"
```

---

## Task 11: Large File Mode

**Files:**
- Create: `src/core/largefile.h` — LargeFileReader class
- Create: `src/core/largefile.cpp` — memory-mapped file reading, line index
- Modify: `src/core/document.h` — threshold constants, large file member
- Modify: `src/core/document.cpp` — large file detection in load()
- Modify: `src/ui/editor.cpp` — large file viewport rendering
- Modify: `CMakeLists.txt` — add largefile source files

### Steps

- [ ] **Step 1: Create LargeFileReader**

Create `src/core/largefile.h`:
```cpp
class LargeFileReader : public QObject
{
    Q_OBJECT
public:
    explicit LargeFileReader(QObject *parent = nullptr);
    ~LargeFileReader();

    bool open(const QString &filePath);
    void close();
    bool isOpen() const;

    // File info
    qint64 fileSize() const;
    qint64 lineCount() const;
    QString encoding() const;

    // Line access
    QString line(qint64 lineNumber) const;
    QStringList lines(qint64 startLine, qint64 count) const;
    qint64 lineStartOffset(qint64 lineNumber) const;

private:
    void buildLineIndex();

    QFile m_file;
    uchar *m_mappedData = nullptr;
    qint64 m_fileSize = 0;
    QVector<qint64> m_lineOffsets;  // byte offset of each line start
    QString m_encoding;
};
```

- [ ] **Step 2: Implement LargeFileReader**

Create `src/core/largefile.cpp`:
- `open()` — open file, `QFileDevice::map()` the entire file, call `buildLineIndex()`
- `buildLineIndex()` — scan mapped data for newline bytes, store offsets in `m_lineOffsets`. For 1GB file this takes ~1s.
- `line(n)` — extract bytes from `m_lineOffsets[n]` to `m_lineOffsets[n+1]`, decode using detected encoding
- `lines(start, count)` — batch version for viewport loading
- `close()` — unmap and close file

- [ ] **Step 3: Add threshold constants and member to Document**

In `src/core/document.h`:
```cpp
static constexpr qint64 LARGE_FILE_THRESHOLD = 10 * 1024 * 1024;
static constexpr qint64 READONLY_FILE_THRESHOLD = 100 * 1024 * 1024;

// Add member
LargeFileReader *m_largeFileReader = nullptr;
bool isLargeFile() const { return m_largeFileReader != nullptr; }
```

- [ ] **Step 4: Modify Document::load() for large files**

In `src/core/document.cpp`, at the start of `load()`:
```cpp
QFileInfo fi(filePath);
if (fi.size() > LARGE_FILE_THRESHOLD) {
    m_largeFileReader = new LargeFileReader(this);
    if (!m_largeFileReader->open(filePath)) {
        delete m_largeFileReader;
        m_largeFileReader = nullptr;
        return false;
    }
    m_filePath = filePath;
    m_encoding = m_largeFileReader->encoding();
    if (fi.size() > READONLY_FILE_THRESHOLD) {
        setReadOnly(true);
    }
    emit filePathChanged(m_filePath);
    return true;
}
// ... existing load logic for normal files
```

- [ ] **Step 5: Adapt Editor for large file mode**

In `src/ui/editor.cpp`, when editor receives a large-file Document:
- Instead of `setPlainText(doc->text())`, load only the first ~1000 lines using `m_largeFileReader->lines(0, 1000)`
- Override scroll behavior: when scrollbar moves, calculate which lines are visible and load them from LargeFileReader
- Connect scrollbar `valueChanged` to a viewport refresh method

This is the most complex step — implement it incrementally. Start with: if large file, load first 1000 lines and display. Scrolling through the rest is a refinement.

- [ ] **Step 6: Update CMakeLists.txt**

Add `src/core/largefile.cpp` to `CORE_SOURCES` and `src/core/largefile.h` to `HEADERS`.

- [ ] **Step 7: Build and test with large file**

```bash
cd build && cmake .. && make -j$(nproc)
# Generate a test file
python3 -c "
for i in range(10_000_000):
    print(f'Line {i}: The quick brown fox jumps over the lazy dog')
" > /tmp/test_large.txt
ls -lh /tmp/test_large.txt  # Should be ~500MB+
./"Olive Notepad" /tmp/test_large.txt &
```

Expected: File opens quickly (not loading full content into memory). Editor shows content. Scrolling works (may be limited to first 1000 lines initially).

- [ ] **Step 8: Commit**

```bash
git add -A && git commit -m "feat: large file mode with memory-mapped reading and line index"
```

---

## Task 12: Search Engine

**Files:**
- Create: `src/core/searchengine.h` — SearchEngine class with modes
- Create: `src/core/searchengine.cpp` — implementation
- Modify: `CMakeLists.txt` — add searchengine source files

### Steps

- [ ] **Step 1: Create SearchEngine header**

Create `src/core/searchengine.h`:
```cpp
#include <QString>
#include <QRegularExpression>
#include <QTextDocument>
#include <QVector>

struct SearchResult {
    QString filePath;  // empty for current document
    int line;
    int column;
    int length;
    QString lineText;
};

class SearchEngine : public QObject
{
    Q_OBJECT
public:
    enum Mode { Normal, Extended, Regex };

    struct Options {
        Mode mode = Normal;
        bool caseSensitive = false;
        bool wholeWord = false;
        bool wrapAround = true;
        bool dotMatchesNewline = false;
        bool searchBackward = false;
    };

    explicit SearchEngine(QObject *parent = nullptr);

    // Single document search
    QVector<SearchResult> findAll(const QString &text, const QString &pattern, const Options &opts);
    SearchResult findNext(const QString &text, int fromPos, const QString &pattern, const Options &opts);

    // Find in files
    void findInFiles(const QString &directory, const QString &pattern,
                     const QString &fileFilter, const Options &opts,
                     bool recursive, bool includeHidden);

    // Replace
    QString replaceMatch(const SearchResult &match, const QString &text,
                        const QString &pattern, const QString &replacement, const Options &opts);

    // Utility
    static QString expandEscapes(const QString &text);  // Extended mode escape expansion
    int matchCount(const QString &text, const QString &pattern, const Options &opts);

signals:
    void fileSearchResult(const SearchResult &result);
    void fileSearchFinished(int totalHits, int totalFiles);
    void fileSearchProgress(const QString &currentFile);

private:
    QRegularExpression buildRegex(const QString &pattern, const Options &opts) const;
    QString preparePattern(const QString &pattern, const Options &opts) const;
};
```

- [ ] **Step 2: Implement SearchEngine**

Create `src/core/searchengine.cpp`:

`expandEscapes()` — convert `\n` → newline, `\t` → tab, `\r` → CR, `\0` → null, `\xNN` → hex byte

`buildRegex()` — based on mode:
- Normal: `QRegularExpression::escape(pattern)`
- Extended: `QRegularExpression::escape(expandEscapes(pattern))`
- Regex: use pattern directly, set `DotMatchesEverythingOption` if `dotMatchesNewline`

Apply `CaseInsensitiveOption` if not caseSensitive. Add `\b` word boundaries if wholeWord.

`findAll()` — use regex to find all matches in text, return vector of SearchResult

`findNext()` — find first match after fromPos (or before if searchBackward)

`findInFiles()` — use `QDirIterator` with `fileFilter` globs. Read each file, call `findAll()`, emit `fileSearchResult` for each match. Run in calling thread (caller should use QThread).

- [ ] **Step 3: Update CMakeLists.txt**

Add `src/core/searchengine.cpp` to `CORE_SOURCES` and `src/core/searchengine.h` to `HEADERS`.

- [ ] **Step 4: Build**

```bash
cd build && cmake .. && make -j$(nproc)
```

Expected: Compiles without errors. SearchEngine is not yet connected to UI.

- [ ] **Step 5: Commit**

```bash
git add -A && git commit -m "feat: SearchEngine with Normal/Extended/Regex modes and find-in-files"
```

---

## Task 13: Tabbed Search Dialog + Search Results Panel + Incremental Search

**Files:**
- Create: `src/ui/searchdialog.h` — tabbed search dialog
- Create: `src/ui/searchdialog.cpp` — dialog implementation
- Create: `src/ui/searchresultspanel.h` — results panel
- Create: `src/ui/searchresultspanel.cpp` — results panel implementation
- Create: `src/ui/incrementalsearchbar.h` — inline search bar
- Create: `src/ui/incrementalsearchbar.cpp` — search bar implementation
- Modify: `src/ui/mainwindow.h` — replace FindDialog with new components
- Modify: `src/ui/mainwindow.cpp` — wire up new search UI
- Remove: `src/ui/finddialog.h`, `src/ui/finddialog.cpp`
- Modify: `CMakeLists.txt` — swap finddialog for new files

### Steps

- [ ] **Step 1: Create SearchResultsPanel**

Create `src/ui/searchresultspanel.h` and `src/ui/searchresultspanel.cpp`:

A `QWidget` containing a `QTreeView` with a custom `QStandardItemModel`:
- Top-level items: file name + hit count (or "Current Document")
- Child items: "Line N: matched content" with line number stored in item data
- Double-click handler emits `resultActivated(QString filePath, int line)`
- Clear button, copy button
- Header label showing "Search 'X' (N hits in M files)"

- [ ] **Step 2: Create SearchDialog**

Create `src/ui/searchdialog.h` and `src/ui/searchdialog.cpp`:

A `QDialog` (non-modal) with `QTabWidget` containing 4 tabs:
- **Find tab**: search input with history combo box, option checkboxes (match case, whole word, wrap around), search mode radio buttons (Normal/Extended/Regex), direction radio buttons (Up/Down), dot-matches-newline checkbox. Buttons: Find Next, Count, Find All in Current Document, Find All in All Open Documents, Close.
- **Replace tab**: same as Find plus replace input and Replace/Replace All buttons.
- **Find in Files tab**: search/replace inputs, directory input with browse button, filter input, recursive/hidden checkboxes. Buttons: Find All, Replace in Files, Close.
- **Mark tab**: search input, options, Bookmark Line checkbox. Buttons: Mark All, Clear All Marks, Close.

Dialog stores a `SearchEngine*` reference. Search history stored as `QStringList` (last 20 entries) in `QComboBox`.

Transparency: `QSlider` in bottom-right corner controlling `setWindowOpacity()`.

Connect all buttons to signals that MainWindow handles.

- [ ] **Step 3: Create IncrementalSearchBar**

Create `src/ui/incrementalsearchbar.h` and `src/ui/incrementalsearchbar.cpp`:

A `QWidget` with horizontal layout: QLabel("Search:"), QLineEdit, next button (▼), prev button (▲), close button (✕), match count label.

Connect QLineEdit `textChanged` to real-time search: highlight all matches using `QTextEdit::ExtraSelection` in the active editor, scroll to first match.

Enter → next match. Shift+Enter → previous match. Escape → close bar.

- [ ] **Step 4: Wire into MainWindow**

In `src/ui/mainwindow.h`:
- Remove `FindDialog *m_findDialog`
- Add `SearchDialog *m_searchDialog`, `SearchResultsPanel *m_searchResultsPanel`, `IncrementalSearchBar *m_incrementalSearchBar`

In `src/ui/mainwindow.cpp`:
- Remove `#include "finddialog.h"`, add new includes
- Create `m_searchResultsPanel` as a `QDockWidget` at bottom (alongside terminal dock)
- Create `m_incrementalSearchBar` and add it to the layout above the tab widget (use `QVBoxLayout` wrapping or insert into main layout)
- Create `m_searchDialog` lazily on first use
- Update `find()`, `replace()`, `findNext()`, `findPrevious()` slots to use new SearchDialog
- Add new slots: `findInFiles()`, `showIncrementalSearch()`, `onSearchResultActivated()`
- Connect SearchDialog signals to MainWindow slots that perform actual search using SearchEngine
- Connect SearchResultsPanel `resultActivated` to open file and go to line

- [ ] **Step 5: Remove old FindDialog files**

```bash
rm src/ui/finddialog.h src/ui/finddialog.cpp
```

- [ ] **Step 6: Update CMakeLists.txt**

Remove `src/ui/finddialog.cpp` from `UI_SOURCES` and `src/ui/finddialog.h` from `HEADERS`.
Add new files: `searchdialog.cpp`, `searchresultspanel.cpp`, `incrementalsearchbar.cpp` to `UI_SOURCES` and corresponding `.h` to `HEADERS`.

- [ ] **Step 7: Build and verify**

```bash
cd build && cmake .. && make -j$(nproc) && ./"Olive Notepad" &
```

Expected: Ctrl+F opens tabbed search dialog on Find tab. Ctrl+H opens on Replace tab. Find All shows results in bottom panel. Ctrl+I shows inline search bar with real-time highlighting. Double-click result navigates to match.

- [ ] **Step 8: Commit**

```bash
git add -A && git commit -m "feat: Notepad++ tabbed search dialog, results panel, incremental search bar"
```

---

## Task 14: Mark System

**Files:**
- Modify: `src/ui/editor.h` — mark highlight storage
- Modify: `src/ui/editor.cpp` — mark/clear mark methods using ExtraSelection
- Modify: `src/ui/searchdialog.cpp` — wire Mark tab buttons

### Steps

- [ ] **Step 1: Add mark state to Editor**

In `src/ui/editor.h`:
```cpp
private:
    QList<QTextEdit::ExtraSelection> m_markSelections;
public:
    void markAll(const QString &pattern, SearchEngine::Options opts);
    void clearMarks();
    int markCount() const { return m_markSelections.size(); }
```

- [ ] **Step 2: Implement mark operations**

In `src/ui/editor.cpp`:

`markAll()` — use SearchEngine to find all matches in current document text. Create `ExtraSelection` for each match with `markHighlightColor` background from theme. Store in `m_markSelections`. Call `setExtraSelections()` combining with bracket highlights and current line highlight.

`clearMarks()` — clear `m_markSelections`, update extra selections.

- [ ] **Step 3: Wire Mark tab in SearchDialog**

In `src/ui/searchdialog.cpp`, connect Mark All button to emit signal. In MainWindow, handle signal by calling `currentEditor()->markAll(...)`. Connect "Bookmark line" option to also call `toggleBookmark()` on each matched line.

- [ ] **Step 4: Build and verify**

Open a file. Use Search > Mark tab to mark a pattern. All matches highlight yellow. Clear Marks removes highlights.

- [ ] **Step 5: Commit**

```bash
git add -A && git commit -m "feat: mark system with persistent highlight and bookmark option"
```

---

## Task 15: Session Persistence for Bookmarks + Folds

**Files:**
- Modify: `src/core/session.h` — no new methods needed, existing save/restore handles it
- Modify: `src/core/session.cpp` — serialize bookmark sets and fold state per document

### Steps

- [ ] **Step 1: Extend session serialization**

In `src/core/session.cpp`, in `windowToJson()`:
- For each open editor tab, add to the document JSON object:
  - `"bookmarks"`: JSON array of bookmark line numbers from `editor->bookmarks()`
  - `"foldedRegions"`: JSON array of folded line numbers (keys from `editor->foldedRegions()`)

- [ ] **Step 2: Extend session restoration**

In `jsonToWindow()`:
- After opening each file and creating the editor, read `"bookmarks"` array and call `editor->toggleBookmark(line)` for each
- Read `"foldedRegions"` array and call `editor->foldAt(line)` for each

- [ ] **Step 3: Build and verify**

Set some bookmarks and fold some regions. Close and reopen the app. Bookmarks and folds should persist.

- [ ] **Step 4: Commit**

```bash
git add -A && git commit -m "feat: persist bookmarks and fold state across sessions"
```

---

## Task 16: Integration Verification

- [ ] **Step 1: Full clean build**

```bash
cd /home/abusulaiman/dev/repository/olive_notepad
rm -rf build && mkdir build && cd build
cmake .. && make -j$(nproc)
```

Expected: 0 errors.

- [ ] **Step 2: Visual verification checklist**

Launch the application and verify each feature:

```bash
./"Olive Notepad" &
```

Verify:
- [ ] Notepad++ light theme (white editor, grey toolbars)
- [ ] Single-row toolbar with 16x16 icons
- [ ] Tab bar shows file-type icons, close on hover, right-click menu has all items
- [ ] Status bar shows: doc type | length/lines | Ln/Col/Sel/Pos | EOL | Encoding | INS/OVR
- [ ] Clicking EOL/Encoding in status bar shows popup menus
- [ ] Menu bar has 12 menus in correct order
- [ ] Three-column gutter: bookmark margin | line numbers | fold margin
- [ ] Click bookmark margin → blue circle, F2 navigates
- [ ] Bracket matching highlights when cursor is next to bracket
- [ ] Ctrl+B jumps to matching bracket
- [ ] Toggle whitespace → dots and arrows visible
- [ ] Toggle EOL → line ending symbols visible
- [ ] Toggle indent guide → vertical dotted lines visible
- [ ] Code folding: click fold margin to fold/unfold functions
- [ ] Alt+drag → column selection with blue rectangle
- [ ] Ctrl+F → tabbed search dialog (Find/Replace/Find in Files/Mark)
- [ ] Find All → results in bottom panel, double-click navigates
- [ ] Ctrl+I → incremental search bar with real-time highlighting
- [ ] Mark tab → highlights all matches yellow
- [ ] Large file: open 500MB+ file, should open without hanging

- [ ] **Step 3: Fix any issues found**

Address any compilation warnings or visual glitches discovered during verification.

- [ ] **Step 4: Final commit**

```bash
git add -A && git commit -m "fix: integration verification fixes for Phase 1"
```

(Only if changes were made in step 3.)

---

## File Map Summary

### New Files (13)
| File | Task |
|------|------|
| `resources/themes/notepadpp.json` | Task 1 |
| `src/ui/columneditor.h` | Task 10 |
| `src/ui/columneditor.cpp` | Task 10 |
| `src/core/largefile.h` | Task 11 |
| `src/core/largefile.cpp` | Task 11 |
| `src/core/searchengine.h` | Task 12 |
| `src/core/searchengine.cpp` | Task 12 |
| `src/ui/searchdialog.h` | Task 13 |
| `src/ui/searchdialog.cpp` | Task 13 |
| `src/ui/searchresultspanel.h` | Task 13 |
| `src/ui/searchresultspanel.cpp` | Task 13 |
| `src/ui/incrementalsearchbar.h` | Task 13 |
| `src/ui/incrementalsearchbar.cpp` | Task 13 |

### Modified Files
| File | Tasks |
|------|-------|
| `src/ui/theme.h` | 1 |
| `src/ui/theme.cpp` | 1 |
| `resources/resources.qrc` | 1 |
| `src/main.cpp` | 1 |
| `src/ui/toolbarmanager.h` | 2 |
| `src/ui/toolbarmanager.cpp` | 2 |
| `src/ui/mainwindow.h` | 2, 3, 5, 13 |
| `src/ui/mainwindow.cpp` | 2, 3, 4, 5, 6, 7, 8, 13, 14 |
| `src/ui/mainwindow_extended.cpp` | 5 |
| `src/ui/tabwidget.h` | 3 |
| `src/ui/tabwidget.cpp` | 3 |
| `src/ui/statusbar.h` | 4 |
| `src/ui/statusbar.cpp` | 4 |
| `src/ui/editor.h` | 6, 7, 8, 9, 10, 14 |
| `src/ui/editor.cpp` | 6, 7, 8, 9, 10, 11 |
| `src/ui/linenumberarea.h` | 6 |
| `src/ui/linenumberarea.cpp` | 6, 9 |
| `src/core/document.h` | 11 |
| `src/core/document.cpp` | 11 |
| `src/core/session.cpp` | 15 |
| `CMakeLists.txt` | 10, 11, 12, 13 |

### Removed Files
| File | Task |
|------|------|
| `src/ui/toolbarmanager_old.cpp` | Pre-flight |
| `src/ui/toolbarmanager.cpp.backup` | Pre-flight |
| `src/ui/finddialog.h` | Task 13 |
| `src/ui/finddialog.cpp` | Task 13 |

# Phase 1 Design: Notepad++ Parity — UI Chrome, Editor Core, Search

**Project:** Olive Notepad
**Date:** 2026-03-25
**Author:** Abu Bakr (Team Lead & Architect)
**Status:** Approved

---

## Overview

Transform Olive Notepad's visual identity, editor core, and search system to match Notepad++ for Windows. This is Phase 1 of a two-phase effort. Phase 2 (syntax highlighting expansion, file handling, plugins) follows after Phase 1 is validated.

### Current State

- ~14,700 lines of C++/Qt across 60+ source/header files
- Compiled binary exists (ELF 64-bit, ~5.8MB)
- Architecture: PieceTable buffer, Document/Editor separation, TabWidget, ToolbarManager, ThemeManager, SyntaxHighlighter, SessionManager, MacroRecorder
- Three-row toolbar with SVG icons, basic Find/Replace dialog, olive-green custom theme
- Many Notepad++ features stubbed but not implemented (folding, whitespace rendering, column mode, indent guides)

### Success Criteria

1. Application visually resembles Notepad++ at first glance — same toolbar density, tab style, status bar layout, gutter structure
2. Editor handles 1GB+ files without degradation
3. Column selection, bracket matching, code folding, and whitespace rendering all functional
4. Search system matches Notepad++ feature set: tabbed dialog, find in files, mark, incremental search, search results panel

---

## Workstream 1: Visual Identity / UI Chrome

### 1.1 Notepad++ Theme

Create a new JSON theme file `resources/themes/notepadpp.json` matching the classic Notepad++ light palette:

| Element | Color |
|---------|-------|
| Toolbar background | `#f0f0f0` |
| Editor background | `#ffffff` |
| Editor foreground | `#000000` |
| Active tab background | `#ffffff` |
| Inactive tab background | `#e8e8e8` |
| Tab border | `#c0c0c0` |
| Status bar background | `#f0f0f0` |
| Status bar foreground | `#000000` |
| Line number foreground | `#808080` |
| Line number background | `#e4e4e4` |
| Current line highlight | `#e8e8ff` |
| Selection background | `#0078d7` (system blue) |
| Fold margin background | `#f0f0f0` |
| Bookmark margin background | `#e4e4e4` |

Default editor font: Consolas 10pt (fallback: Courier New, monospace).

Keyword/syntax colors follow Notepad++ defaults:
- Keyword: `#0000ff` (blue)
- String: `#808080` (grey) — matches Notepad++ default; same value as line number foreground, which is faithful but low-contrast for strings
- Comment: `#008000` (green)
- Number: `#ff8000` (orange)
- Operator: `#000080` (dark blue)
- Preprocessor: `#804000` (brown)

**New Theme struct fields** (added to `src/ui/theme.h`):

| Field | Type | Purpose |
|-------|------|---------|
| `indentGuideColor` | `QColor` | Vertical indent guide lines |
| `whitespaceColor` | `QColor` | Space dots, tab arrows, EOL symbols |
| `foldMarginBackground` | `QColor` | Fold margin gutter background |
| `bookmarkMarginBackground` | `QColor` | Bookmark margin gutter background |
| `markHighlightColor` | `QColor` | Search mark highlight background |
| `bracketMatchBackground` | `QColor` | Matched bracket highlight |
| `bracketErrorBackground` | `QColor` | Unmatched bracket highlight (red) |

### 1.2 Single-Row Toolbar

Replace the current 3-row toolbar layout with a single dense row matching Notepad++:

```
[New][Open][Save][SaveAll][Close][CloseAll] | [Cut][Copy][Paste][Undo][Redo] | [Find][Replace][FindInFiles] | [ZoomIn][ZoomOut] | [WordWrap][ShowWhitespace][ShowEOL][IndentGuide] | [Record][Stop][Play] | [Comment][Indent][Outdent]
```

- Icon size: 16x16 (down from current 20x20)
- No toolbar text labels — icons only with tooltips
- Toolbar is not movable (fixed at top, matching N++ default behavior)
- Single `addToolBar()` call, no `addToolBarBreak()`

**Files modified:** `src/ui/toolbarmanager.cpp`, `src/ui/toolbarmanager.h`

### 1.3 Enhanced Tab Bar

Modify `TabBar`/`TabWidget` to match Notepad++ tab behavior:

- **File-type icons on tabs** — use a mapping from file extension to a small colored icon (`.cpp` → blue, `.py` → green, `.js` → yellow, `.html` → orange, etc.). Plain text gets a generic document icon.
- **Close button** — small X on each tab, visible on hover or active tab only (not always visible on inactive tabs)
- **Modified indicator** — red floppy disk icon replaces normal icon when file is modified (Notepad++ style)
- **Double-click empty area** — creates new tab
- **Middle-click** — closes tab (already implemented)
- **Right-click context menu** — Close, Close All But This, Close All to the Left, Close All to the Right, Save, Save As, Copy Full Path, Open Containing Folder (extend current menu)
- **Tab overflow** — scroll arrows when tabs exceed window width (Notepad++ behavior)

**Files modified:** `src/ui/tabwidget.cpp`, `src/ui/tabwidget.h`

**Specific changes to `tabwidget.h`:** Add `closeToLeftRequested(int index)` signal and `onCloseToLeftRequested(int index)` slot. Add file-type icon mapping. Add close-button-on-hover behavior.

### 1.4 Notepad++ Menu Structure

Restructure menus to match Notepad++:

```
File | Edit | Search | View | Encoding | Language | Settings | Tools | Macro | Run | Window | ?
```

**New menus to add:**

- **Encoding** — Convert to ANSI, UTF-8, UTF-8-BOM, UCS-2 BE/LE. Current encoding checkmarked. (Move from Edit menu where encoding operations currently live)
- **Language** — List of supported languages with current one checkmarked. Auto-detect option at top. Separator between language categories.
- **Settings** — Preferences, Style Configurator, Shortcut Mapper (Preferences dialog already exists, others are stubs for now)
- **Run** — Launch in terminal, open containing folder, open command prompt here
- **Window** — List of open documents for quick switching. Windows dialog for managing all open files.

**Reorganize existing menus:**
- **Edit** — add missing Notepad++ items: Column Editor, Character Panel, Clipboard History, Set Read-Only, Clear Read-Only flag
- **View** — add: Always on Top, Toggle Full Screen, Post-It mode (distraction-free), Summary, Fold All/Unfold All, Document Map (stub for Phase 2), Function List (stub for Phase 2)

**Files modified:** `src/ui/mainwindow.cpp` (setupMenus)

### 1.5 Enhanced Status Bar

Redesign StatusBarWidget to match Notepad++ layout (left to right):

```
[Document type] | [Length: X  Lines: Y] | [Ln: X  Col: Y  Sel: Z  Pos: W] | [CR LF / LF / CR] | [UTF-8 / ANSI / etc.] | [INS / OVR]
```

- **Document type** — shows file type icon + language name
- **Length/Lines** — total character count and line count
- **Position** — line, column, selection character count, absolute position
- **EOL indicator** — clickable, opens EOL conversion menu
- **Encoding indicator** — clickable, opens encoding conversion menu
- **Insert/Overwrite mode** — toggles on Insert key press, clickable

Each section is a separate clickable QLabel or QPushButton with flat styling. Sections separated by sunken panel borders (QFrame::Panel | QFrame::Sunken) matching the classic Notepad++ look.

**Files modified:** `src/ui/statusbar.cpp`, `src/ui/statusbar.h`

### 1.6 Three-Column Gutter

Extend the editor gutter from a single line-number column to three columns:

```
[Bookmark margin (16px)] | [Line numbers (variable)] | [Fold margin (16px)]
```

- **Bookmark margin** — click to toggle bookmark (blue circle indicator). Visible by default.
- **Line numbers** — existing implementation, adjusted for new position offset.
- **Fold margin** — clickable `[-]`/`[+]` fold indicators. Connected to the code folding system (Workstream 2).

The gutter is painted as a single widget (`LineNumberArea`) with three logical sections, not three separate widgets.

**Files modified:** `src/ui/linenumberarea.h`, `src/ui/linenumberarea.cpp`, `src/ui/editor.cpp`, `src/ui/editor.h`

---

## Workstream 2: Editor Core Hardening

### 2.1 Large File Mode

For files >10MB, bypass the PieceTable and use a viewport-rendering approach:

- **Memory-mapped file** — use `QFile` + `mmap()` (or `QFileDevice::map()`) to map the file into memory without loading it entirely into a QString.
- **Block-based loading** — divide the file into blocks (~64KB each). Only decode and load blocks that are visible in the viewport plus a small buffer zone above and below.
- **Read-only by default** — files >100MB open in read-only mode with an "Enable Editing" button in the status bar. When editing is enabled, modified blocks are tracked in an overlay structure.
- **Line index** — on open, scan the file once to build a line-start-offset index (this is fast — ~1s for 1GB). This enables instant line navigation.
- **Smooth scrolling** — scrollbar maps to the line index, not the loaded QTextDocument blocks.

Threshold constants defined in `src/core/document.h`:
```cpp
static constexpr qint64 LARGE_FILE_THRESHOLD = 10 * 1024 * 1024;      // 10MB
static constexpr qint64 READONLY_FILE_THRESHOLD = 100 * 1024 * 1024;   // 100MB
static constexpr qint64 VIEWPORT_BLOCK_SIZE = 64 * 1024;               // 64KB
```

**New file:** `src/core/largefile.h`, `src/core/largefile.cpp`
**Files modified:** `src/core/document.cpp`, `src/core/document.h`, `src/ui/editor.cpp`

### 2.2 Column / Block Selection

- **Alt+Mouse drag** — creates a rectangular (column) selection across multiple lines. Visual highlight is a rectangle, not line-by-line.
- **Alt+Shift+Arrow** — keyboard column selection.
- **Typing with column selection** — inserts text at each line's column position simultaneously.
- **Column Editor dialog** — accessible via Edit > Column Editor (Alt+C):
  - Insert text at column position across selected lines
  - Insert sequential numbers (start, step, leading zeros option)

Implementation: override `mouseMoveEvent` and `keyPressEvent` in Editor. For column selection visual highlight, install an event filter on `viewport()` via `viewport()->installEventFilter(this)` and handle `QEvent::Paint` in `eventFilter()` — direct `paintEvent` override on the Editor class draws behind the viewport and produces nothing visible. Store column selection as `{startLine, startCol, endLine, endCol}` separate from QTextCursor.

**Files modified:** `src/ui/editor.cpp`, `src/ui/editor.h`
**New file:** `src/ui/columneditor.h`, `src/ui/columneditor.cpp`

### 2.3 Bracket Matching

- On cursor movement, check if cursor is adjacent to `()[]{}`. If so, find the matching bracket and highlight both with a colored rectangle background.
- **Ctrl+B** — jump to matching bracket.
- Matching logic handles nested brackets correctly with a stack-based approach.
- Unmatched brackets get a red highlight to indicate error.
- Respects string/comment context (don't match brackets inside strings or comments) — use the syntax highlighter's state information.

Implementation: connect to `cursorPositionChanged`, scan for brackets, use `QTextEdit::ExtraSelection` for highlighting.

**Files modified:** `src/ui/editor.cpp`, `src/ui/editor.h`

### 2.4 Indent Guide Rendering

- Paint vertical dotted lines at each tab-stop column in the editor's `paintEvent`.
- Lines are drawn from the top of the visible area to the bottom, one per indent level that is actually used on visible lines.
- Color: subtle grey (`#d0d0d0` in light theme, `#404040` in dark theme) — themeable via `Theme::indentGuideColor`.
- Toggle via View > Indent Guide (current slot exists, needs rendering).

**Files modified:** `src/ui/editor.cpp`, `src/ui/theme.h`, `src/ui/theme.cpp`

### 2.5 Whitespace / EOL Visualization

- **Spaces** — rendered as centered dot `·` (U+00B7) in a subtle color.
- **Tabs** — rendered as right arrow `→` (U+2192) spanning the tab width.
- **Line endings** — `LF` shown as `↓`, `CRLF` shown as `CR LF` or `↵`, `CR` shown as `←`. Displayed at end of each line.
- Toggle independently: Show Whitespace (spaces+tabs), Show EOL (line endings).
- Color: lighter than normal text — themeable via `Theme::whitespaceColor`.

Implementation: custom painting in `paintEvent` overlay, not by inserting actual characters.

**Files modified:** `src/ui/editor.cpp`, `src/ui/editor.h`, `src/ui/theme.h`

### 2.6 Code Folding

- **Fold margin** — 16px column in the gutter showing `[-]` for expanded foldable blocks, `[+]` for collapsed blocks, vertical line connecting fold range.
- **Click to fold/unfold** — clicking the fold indicator toggles the fold state.
- **Fold detection:**
  - Brace-based: `{` opens a fold region, matching `}` closes it. For C, C++, Java, JS, etc.
  - Indent-based: increase in indent level opens a fold region. For Python, YAML, etc.
  - Comment blocks: multi-line comments are foldable.
- **Fold All / Unfold All** — menu and keyboard shortcuts.
- **Fold level controls** — fold to level 1, 2, 3, etc. (Alt+1 through Alt+8).
- **Persistent fold state** — save/restore fold state with session.
- **Folded text indicator** — when a block is folded, show `[...]` inline after the fold line.

**Implementation note:** `QTextBlock::setVisible(false)` is unreliable in Qt6's `QPlainTextEdit` — it leaves visual gaps instead of reflowing the layout. Instead, use the block-caching approach: on fold, remove folded blocks from the QTextDocument and cache their content in a `QMap<int, QStringList>` keyed on the header block number; on unfold, re-insert the cached blocks. The fold header line gets a `[...]` suffix appended while folded. Store the fold map and a set of folded block numbers in the Editor. Fold margin rendering in `LineNumberArea::paintEvent`.

**Files modified:** `src/ui/editor.cpp`, `src/ui/editor.h`, `src/ui/linenumberarea.cpp`

### 2.7 Bookmark System

- **Bookmark margin** — click the bookmark margin (leftmost gutter column) to toggle a bookmark on that line.
- **Visual indicator** — filled blue circle in the bookmark margin.
- **Navigation** — F2 (next bookmark), Shift+F2 (previous bookmark).
- **Clear all bookmarks** — menu action.
- **Bookmark persistence** — saved/restored with session.

Store bookmarks as a `QSet<int>` of block numbers in Editor. Paint in the bookmark margin section of `LineNumberArea`.

**Files modified:** `src/ui/editor.cpp`, `src/ui/editor.h`, `src/ui/linenumberarea.cpp`

---

## Workstream 3: Search System

### 3.1 Tabbed Search Dialog

Replace `FindDialog` with a new `SearchDialog` class using QTabWidget with four tabs:

**Find tab:**
```
Find what: [___________________] [v history dropdown]
  [ ] Match whole word only    ( ) Normal
  [ ] Match case               ( ) Extended (\n, \t, \0, \x...)
  [ ] Wrap around              ( ) Regular expression
  Direction: ( ) Up  (x) Down     [ ] . matches newline

[Find Next] [Count] [Find All in Current Document] [Find All in All Open Documents] [Close]
```

**Replace tab:**
```
Find what:    [___________________]
Replace with: [___________________]
  (same options as Find tab)

[Find Next] [Replace] [Replace All] [Replace All in All Open Documents] [Close]
```

**Find in Files tab:**
```
Find what: [___________________]
Replace with: [___________________]
Directory:  [___________________] [...]
Filters:    [___________________]  (e.g. *.cpp;*.h)
  [ ] In all sub-folders
  [ ] In hidden folders
  [ ] Match whole word only
  [ ] Match case

[Find All] [Replace in Files] [Close]
```

**Mark tab:**
```
Find what: [___________________]
  [ ] Match whole word only
  [ ] Match case
  [ ] Bookmark line
  ( ) Normal  ( ) Extended  ( ) Regular expression

[Mark All] [Clear All Marks] [Close]
```

**Dialog behavior:**
- Non-modal (stays open while editing)
- Remembers last search/replace text and options
- Search history dropdown (last 20 searches)
- Transparency slider in bottom-right (10%-100%, affects dialog opacity on focus loss or always)

**New files:** `src/ui/searchdialog.h`, `src/ui/searchdialog.cpp`
**Removed:** `src/ui/finddialog.h`, `src/ui/finddialog.cpp` (replaced entirely)

### 3.2 Search Modes

- **Normal** — literal text matching.
- **Extended** — parse escape sequences: `\n` (newline), `\r` (CR), `\t` (tab), `\0` (null), `\xNN` (hex byte). Convert the search string before matching.
- **Regex** — use `QRegularExpression`. Support backreferences in replace (`\1`, `$1`). Option for `. matches newline` (DOTALL flag).

Implementation: a `SearchEngine` utility class that takes mode + options and returns match results. Used by both the dialog and the incremental search bar.

**New file:** `src/core/searchengine.h`, `src/core/searchengine.cpp`

### 3.3 Search Results Panel

A dockable bottom panel (`QDockWidget`) that displays search results:

```
Search "pattern" (N hits in M files)
  path/to/file.cpp (3 hits)
    Line 42:   matched line content here
    Line 87:   another matched line
    Line 153:  third match
  path/to/other.py (1 hit)
    Line 12:   matched content
```

- **QTreeView** with a custom model (file grouping → line results).
- **Double-click** a result line to navigate to that file and line in the editor.
- **Copy** result lines to clipboard.
- **Clear** button to clear results.
- Panel replaces the current terminal dock position when visible (both are bottom docks, only one shown at a time, or they can be tabbed).

**New files:** `src/ui/searchresultspanel.h`, `src/ui/searchresultspanel.cpp`

### 3.4 Find in Files

- **Directory picker** with browse button and MRU directory history.
- **File filter** — semicolon-separated glob patterns (e.g., `*.cpp;*.h;*.txt`).
- **Recursive** — checkbox to search subdirectories.
- **Hidden folders** — checkbox to include hidden directories.
- **Replace in Files** — find and replace across files on disk with confirmation count before executing.

Implementation: runs in a background `QThread` to avoid blocking the UI. Emits results incrementally to the SearchResultsPanel. Uses `QDirIterator` for file enumeration and reads files in chunks for memory efficiency.

**Files modified:** `src/core/searchengine.cpp` (add file search methods)

### 3.5 Mark System

- **Mark All** — highlights all matches of the search text with a persistent colored background (distinct from selection highlight — uses yellow/orange by default, themeable).
- **Bookmark marked lines** — option to add bookmarks to all lines containing a match.
- **Clear All Marks** — removes all mark highlights.
- Multiple mark styles possible (Notepad++ supports 5 mark colors — implement 1 for now, extendable later).

Implementation: use `QTextEdit::ExtraSelection` with a dedicated mark style stored in the Editor.

**Files modified:** `src/ui/editor.cpp`, `src/ui/editor.h`

### 3.6 Incremental Search Bar

A slim toolbar-style bar that appears below the main toolbar (or above the editor area) when triggered by Ctrl+I (note: Notepad++ uses Ctrl+Alt+I on Windows, but Ctrl+Alt combos conflict with Linux WM shortcuts on GNOME/KDE; Ctrl+I is the safer default, user-configurable via Preferences):

```
[Search: [_______________] [v] [^] [X close]]
```

- **Real-time highlighting** — as you type, all matches are highlighted in the document, current match is scrolled into view.
- **Enter / Shift+Enter** or arrow buttons — next/previous match.
- **Escape** — close bar, return focus to editor.
- **Match count** displayed (e.g., "3 of 17").

Separate from the main Search dialog. Lightweight, fast, no options beyond the search text.

**New file:** `src/ui/incrementalsearchbar.h`, `src/ui/incrementalsearchbar.cpp`

### 3.7 Regex Support in Editor

Current `Editor::findNext` uses `QTextDocument::find(QString)`. Add a parallel path:

- When regex mode is active, use `QRegularExpression` to search the document text.
- For large documents, search the visible viewport first for responsiveness, then extend to full document.
- Replace supports backreferences (`\1`, `$1`, `$&`, etc.).

**Files modified:** `src/ui/editor.cpp`

---

## Files Summary

### New Files (13)
| File | Purpose |
|------|---------|
| `src/core/largefile.h` | Large file memory-mapped reader |
| `src/core/largefile.cpp` | Large file implementation |
| `src/core/searchengine.h` | Search engine (modes, find in files) |
| `src/core/searchengine.cpp` | Search engine implementation |
| `src/ui/searchdialog.h` | Tabbed search dialog (replaces FindDialog) |
| `src/ui/searchdialog.cpp` | Search dialog implementation |
| `src/ui/searchresultspanel.h` | Search results bottom panel |
| `src/ui/searchresultspanel.cpp` | Search results implementation |
| `src/ui/incrementalsearchbar.h` | Inline incremental search bar |
| `src/ui/incrementalsearchbar.cpp` | Incremental search implementation |
| `src/ui/columneditor.h` | Column editor dialog |
| `src/ui/columneditor.cpp` | Column editor implementation |
| `resources/themes/notepadpp.json` | Notepad++ classic theme |

### Modified Files (17)
| File | Changes |
|------|---------|
| `src/ui/mainwindow.h` | Remove `FindDialog *m_findDialog` member and forward decl; add `SearchDialog *m_searchDialog`, `SearchResultsPanel *m_searchResultsPanel`, `IncrementalSearchBar *m_incrementalSearchBar`; remove six `QToolBar*` members, replace with single `QToolBar *m_mainToolBar`; add new menu pointers for Encoding, Language, Settings, Run, Window menus |
| `src/ui/mainwindow.cpp` | Remove `#include "finddialog.h"`, add new includes; menu restructure to Notepad++ layout; new signal/slot connections for all new UI components |
| `src/ui/mainwindow_extended.cpp` | New slot implementations for added menu items |
| `src/ui/toolbarmanager.h` | Single `QToolBar*` member instead of six; remove per-toolbar accessors |
| `src/ui/toolbarmanager.cpp` | Single-row toolbar layout, remove `addToolBarBreak()` calls |
| `src/ui/tabwidget.h` | Add `closeToLeftRequested(int)` signal, `onCloseToLeftRequested(int)` slot; file-type icon mapping; close-button-on-hover behavior |
| `src/ui/tabwidget.cpp` | Tab bar enhancements, extended context menu |
| `src/ui/statusbar.h` | New sections (doc type, length/lines, sel count, INS/OVR), clickable indicators |
| `src/ui/statusbar.cpp` | Enhanced status bar layout with sunken panel borders |
| `src/ui/editor.h` | Column selection, bracket matching, folding (fold map), whitespace rendering, bookmarks, marks, viewport event filter |
| `src/ui/editor.cpp` | Core editor enhancements, viewport event filter for column selection overlay |
| `src/ui/linenumberarea.h` | Three-column gutter (bookmark, line numbers, fold margin) |
| `src/ui/linenumberarea.cpp` | Bookmark margin, fold margin rendering |
| `src/ui/theme.h` | New color fields: `indentGuideColor`, `whitespaceColor`, `foldMarginBackground`, `bookmarkMarginBackground`, `markHighlightColor`, `bracketMatchBackground`, `bracketErrorBackground` |
| `src/ui/theme.cpp` | New theme fields, JSON serialization, notepadpp theme |
| `src/core/document.h` | Large file threshold constants, `LargeFileReader *m_largeFileReader = nullptr` member, `bool isLargeFile() const` accessor |
| `src/core/document.cpp` | Large file mode detection in `load()`, delegate to LargeFileReader when threshold exceeded |
| `src/core/session.h` | Add serialization for per-document fold state (`QMap<int, bool>`) and bookmark set (`QSet<int>`) |
| `src/core/session.cpp` | Persist fold state and bookmarks in session JSON |
| `CMakeLists.txt` | Add new source files, remove finddialog sources |
| `resources/resources.qrc` | Add `notepadpp.json` theme entry |

### Removed Files (4)
| File | Reason |
|------|--------|
| `src/ui/finddialog.h` | Replaced by SearchDialog |
| `src/ui/finddialog.cpp` | Replaced by SearchDialog |
| `src/ui/toolbarmanager_old.cpp` | Dead code — stale backup not in CMakeLists.txt |
| `src/ui/toolbarmanager.cpp.backup` | Dead code — stale backup not in CMakeLists.txt |

---

## What Is NOT Being Built (Phase 1)

- Document Map / Minimap — Phase 2
- Function List panel — Phase 2
- Syntax highlighting expansion (80+ languages) — Phase 2
- Plugin architecture — Phase 2
- LSP integration — Phase 2
- Git integration — Phase 2
- Remote connections — Phase 2
- Hex viewer — exists, no changes
- Markdown preview — exists, no changes
- Auto-completion / snippets — Phase 2
- TextMate grammar support — Phase 2
- Multi-cursor (Ctrl+click) — deferred; column selection covers the primary use case. Multi-cursor is additive on top of column mode.

---

## Build Sequence

1. **Theme + toolbar** — lowest risk, immediately visible, validates the build system still works
2. **Tab bar + status bar + menu restructure** — completes the UI chrome
3. **Gutter + bookmarks** — three-column gutter (bookmark margin, line numbers, fold margin) AND bookmark data model (`QSet<int>` in Editor, F2 navigation, click-to-toggle). Bookmark visual and data must land together.
4. **Editor core** — bracket matching, indent guides, whitespace rendering (no external dependencies)
5. **Code folding** — depends on gutter fold margin being in place
6. **Column selection + column editor** — independent editor feature
7. **Large file mode** — independent, can be tested with generated large files
8. **Search engine** — core search logic, no UI dependency
9. **Search dialog + results panel + incremental bar** — UI on top of search engine
10. **Mark system** — uses search engine + editor extra selections
11. **Integration test** — full pass verifying all features work together

Each step produces a compilable, runnable binary. No step depends on a later step.

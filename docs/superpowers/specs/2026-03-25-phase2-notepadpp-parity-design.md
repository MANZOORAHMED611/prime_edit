# Phase 2 Design: Notepad++ Parity — Syntax, File Handling, N++ Features

**Project:** Olive Notepad
**Date:** 2026-03-25
**Author:** Abu Bakr (Team Lead & Architect)
**Status:** Approved
**Prerequisite:** Phase 1 complete (UI chrome, editor core, search system)

---

## Overview

Complete Notepad++ feature parity with three workstreams: expand syntax highlighting from 13 hardcoded languages to 80+ via data-driven definitions, harden file handling with robust encoding detection and persistent unsaved documents, and add Notepad++ specific features (document map, function list, auto-completion, macro enhancements, plugin wiring).

### Current State (Post-Phase 1)

- ~16,634 lines of C++/Qt across 70+ source/header files
- 6.1MB compiled binary, clean build
- 13 languages with hardcoded highlighting rules in `highlighter.cpp` (793 lines)
- LanguageManager maps ~60 extensions to ~40 languages (only 13 have rules)
- Encoding class: basic UTF-8/BOM encode/decode (~114 lines)
- MacroRecorder: record/playback/save/load (~214 lines)
- PluginInterface + PluginManager: full lifecycle, not wired to UI (~242 lines)
- DocumentManager: auto-recovery timer, recent files, no persistent unsaved docs

### Success Criteria

1. 80+ languages with syntax highlighting, loaded from JSON definitions
2. Accurate encoding detection for non-Unicode files (Windows-1252, ISO-8859, CJK)
3. Unsaved/untitled documents persist silently across sessions — Notepad++'s signature feature
4. File change monitoring with reload prompt
5. Document map minimap, function list panel, and auto-completion working
6. Macro run-N-times, plugin menu with startup loading

---

## Workstream 1: Syntax Highlighting Expansion

### 1.1 Data-Driven Syntax Definitions

Replace the 13 hardcoded `loadXxxRules()` methods with a generic JSON-based loader. Each language definition is a JSON file in `resources/syntax/`.

**JSON definition format:**

```json
{
  "name": "C++",
  "extensions": ["cpp", "cxx", "cc", "hpp", "hxx", "hh", "h"],
  "keywords": ["auto", "break", "case", "class", "const", "..."],
  "types": ["int", "float", "double", "char", "void", "bool", "..."],
  "builtins": ["sizeof", "alignof", "typeid", "..."],
  "singleLineComment": "//",
  "multiLineCommentStart": "/*",
  "multiLineCommentEnd": "*/",
  "stringDelimiters": ["\"", "'"],
  "rawStringPrefix": "R\"(",
  "rawStringSuffix": ")\"",
  "preprocessorPrefix": "#",
  "numberPattern": "\\b[0-9]+(\\.[0-9]+)?([eE][+-]?[0-9]+)?\\b|\\b0[xX][0-9a-fA-F]+\\b",
  "operatorPattern": "[+\\-*/%=<>&|!^~?:]+",
  "functionPattern": "\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(",
  "foldBased": "brace",
  "functionRegex": "^\\s*(?:(?:static|virtual|inline|explicit|constexpr)\\s+)*(?:[\\w:*&<>]+\\s+)+([\\w:]+)\\s*\\("
}
```

Key fields:
- `keywords`, `types`, `builtins` — word lists, highlighted with different formats
- `singleLineComment`, `multiLineCommentStart/End` — comment patterns
- `stringDelimiters` — characters that start/end strings
- `preprocessorPrefix` — line-start prefix for preprocessor directives
- `numberPattern`, `operatorPattern` — regex patterns (optional, defaults provided)
- `functionPattern` — regex for function call detection (for highlighting)
- `foldBased` — `"brace"` or `"indent"` (used by code folding)
- `functionRegex` — regex for function list panel extraction (Workstream 3)

**Files:**
- Modify: `src/syntax/highlighter.h` — remove 13 `loadXxxRules()` declarations, add `loadFromDefinition(const QJsonObject &)`
- Modify: `src/syntax/highlighter.cpp` — replace all hardcoded methods with single generic loader
- Create: `resources/syntax/*.json` — 80+ language definition files
- Modify: `resources/resources.qrc` — add all syntax definition files
- Modify: `src/syntax/languagemanager.h` — add `definitionForLanguage(const QString &)` returning path to JSON
- Modify: `src/syntax/languagemanager.cpp` — register all JSON definitions, map language names to resource paths

### 1.2 Theme-Aware Syntax Colors

The current highlighter uses hardcoded VS Code dark theme colors. Change to read from the current Theme.

In `SyntaxHighlighter::setupFormats()`, replace hardcoded `QColor(86, 156, 214)` etc. with:
```cpp
Theme theme = ThemeManager::instance().currentTheme();
m_keywordFormat.setForeground(theme.keyword);
m_stringFormat.setForeground(theme.string);
// ... etc.
```

Connect to theme changes so switching themes rehighlights all documents.

**Files modified:** `src/syntax/highlighter.cpp`

### 1.3 Language Definitions (80+)

Create JSON definitions for all languages in the requirements spec, grouped by category:

| Category | Languages |
|----------|-----------|
| Systems | C, C++, Rust, Go, Assembly, D, Zig, Nim |
| Web | HTML, CSS, SCSS, LESS, JavaScript, TypeScript, PHP |
| Scripting | Python, Ruby, Perl, Bash, PowerShell, Lua, Tcl |
| Data | JSON, YAML, XML, TOML, INI, CSV |
| Markup | Markdown, LaTeX, reStructuredText |
| Database | SQL, PostgreSQL, MySQL |
| Config | Nginx, Apache, Dockerfile, Makefile, CMake |
| JVM | Java, Kotlin, Scala, Groovy, Clojure |
| .NET | C#, F#, Visual Basic |
| Functional | Haskell, OCaml, Erlang, Elixir, Scheme, Lisp |
| Mobile | Swift, Objective-C, Dart |
| Scientific | R, MATLAB, Julia, Fortran |
| Legacy | COBOL, Pascal, Ada |
| Hardware | Verilog, VHDL |
| Other | Prolog, Vim, Fish, Git (diff/ignore) |

Each file: `resources/syntax/<language-lowercase>.json`

The 13 existing hardcoded languages (C++, Python, JavaScript, HTML, CSS, JSON, YAML, Markdown, Bash, Rust, Go, Java, SQL) are converted to JSON first — they serve as the template for all others.

### 1.4 Auto-Detection Improvements

Extend `LanguageManager::detectLanguage()` with additional content heuristics:
- LaTeX: `\documentclass` or `\begin{document}`
- Makefile: starts with target rules (`target:`)
- CMake: `cmake_minimum_required`
- Dockerfile: `FROM` as first instruction
- XML: `<?xml`

**Files modified:** `src/syntax/languagemanager.cpp`

---

## Workstream 2: File Handling Hardening

### 2.1 Robust Encoding Detection

Current detection is BOM-only. Add a heuristic charset detector as fallback.

**Implementation:** byte-frequency analysis on the first 8KB of the file:
- Check for BOM first (existing)
- If no BOM: check if valid UTF-8 — if all bytes decode cleanly, assume UTF-8
- If not valid UTF-8: analyze byte distribution:
  - High-byte frequencies in 0x80-0x9F range → Windows-1252
  - High-byte frequencies in 0xA0-0xFF range → ISO-8859-1
  - Specific byte patterns for Shift-JIS (0x81-0x9F, 0xE0-0xEF lead bytes)
  - Specific patterns for GB2312/GBK (0xA1-0xF7 lead bytes)
  - Specific patterns for Big5 (0xA1-0xF9 lead bytes)
  - KOI8-R detection via Cyrillic character frequencies

This is a best-effort detector — not a full ICU replacement. Sufficient for Notepad++ parity which also uses heuristics.

**New file:** `src/core/charsetdetector.h`, `src/core/charsetdetector.cpp`
**Files modified:** `src/core/document.cpp` (use detector in `load()`), `CMakeLists.txt`

### 2.2 Encoding Conversion Hardening

The Encoding class has basic encode/decode. Extend to support all declared encodings using Qt's `QStringEncoder`/`QStringDecoder` (Qt6) or `QTextCodec` (Qt5):

- Windows-1252, ISO-8859-1 through ISO-8859-15
- KOI8-R, KOI8-U
- Shift-JIS, EUC-JP, ISO-2022-JP
- GB2312, GBK, GB18030
- Big5
- UTF-16 LE/BE with and without BOM
- UTF-32 LE/BE

**Files modified:** `src/core/encoding.cpp`

### 2.3 File Change Monitoring

Use `QFileSystemWatcher` to monitor all open files for external modifications.

**Implementation:**
- In `DocumentManager`, maintain a `QFileSystemWatcher`
- When a file is opened, add its path to the watcher
- When a file is closed, remove it
- On `fileChanged` signal, check if the file's timestamp/size has changed
- Emit a signal that MainWindow catches to show a notification bar

**Notification bar:** A `QWidget` inserted at the top of the editor area (above the editor, below the incremental search bar) with message: "This file has been modified by another program. Do you want to reload it?" and [Yes] [No] buttons. Styled as a yellow info bar matching Notepad++.

**New file:** `src/ui/notificationbar.h`, `src/ui/notificationbar.cpp`
**Files modified:** `src/core/documentmanager.h`, `src/core/documentmanager.cpp`, `src/ui/mainwindow.cpp`, `CMakeLists.txt`

### 2.4 Recovery Dialog

On startup, before creating the main window, check if recovery files exist in `~/.local/share/OliveNotepad/recovery/`. If found, show a dialog:

```
Olive Notepad was not properly closed. The following documents can be recovered:

[x] Untitled 1 (2026-03-25 14:32)
[x] /home/user/code/main.cpp (modified, 2026-03-25 14:30)
[ ] Untitled 2 (2026-03-25 13:15)

[Recover Selected] [Discard All]
```

**New file:** `src/ui/recoverydialog.h`, `src/ui/recoverydialog.cpp`
**Files modified:** `src/main.cpp` (show dialog before MainWindow), `CMakeLists.txt`

### 2.5 Reload from Disk

Add File > Reload from Disk (Ctrl+Shift+R) action:
- If file is unmodified, reload silently
- If modified, prompt: "Are you sure? Unsaved changes will be lost."
- Re-detect encoding and language after reload

**Files modified:** `src/ui/mainwindow.h`, `src/ui/mainwindow.cpp`

### 2.6 Persistent Unsaved Documents

**The signature Notepad++ feature.** When the application closes:

1. **Untitled tabs with content** — serialize the full text content, cursor position, and tab title to `~/.local/share/OliveNotepad/sessions/unsaved/`. Each gets a UUID-named file.

2. **Named files with unsaved modifications** — serialize the modified content (not the on-disk version) alongside the file path. On restore, open the file but replace its content with the cached version and mark as modified.

3. **No save prompt for session-persisted documents** — Modify `MainWindow::closeEvent()` to suppress the "Save before closing?" dialog. Instead, silently persist all unsaved state. The prompt only appears for explicit close-tab actions (Ctrl+W), not for application quit.

4. **Restore on launch** — In `Session::restore()`, after restoring normal session files, also restore unsaved documents from the cache directory. They appear exactly as they were.

**Implementation details:**
- Cache format: JSON file per unsaved document containing `{text, cursorPosition, cursorAnchor, scrollPosition, title, originalFilePath, encoding, language}`
- Cache directory: `~/.local/share/OliveNotepad/sessions/unsaved/`
- On clean close (all saved), the unsaved cache directory is emptied
- On crash recovery, unsaved cache files survive and are restored via the Recovery Dialog (2.4)

**Files modified:**
- `src/core/session.h` — add unsaved document persistence methods
- `src/core/session.cpp` — implement save/restore for unsaved documents
- `src/ui/mainwindow.cpp` — modify `closeEvent()` to persist instead of prompting

### 2.7 Print System Hardening

Complete the print system:
- **Print preview** — `QPrintPreviewDialog` with syntax highlighting in output
- **Headers/footers** — configurable via Preferences: filename, date, page number, custom text
- **Margins** — configurable top/bottom/left/right
- **Line numbers in print** — optional, toggleable in print dialog

**Files modified:** `src/ui/mainwindow.cpp` (printFile method), add new `src/ui/printdialog.h`/`.cpp` if needed, or use `QPrintPreviewDialog` directly.

---

## Workstream 3: Notepad++ Specific Features

### 3.1 Document Map (Minimap)

A right-side dock widget showing a zoomed-out view of the entire document.

**Implementation:**
- `DocumentMapWidget` — a `QPlainTextEdit` subclass in read-only mode
- Font: 1pt (or smallest legible), no line numbers, no gutter
- Synced to the active editor's content via `setPlainText(editor->toPlainText())`
- **Viewport highlight** — draw a semi-transparent rectangle overlay showing which portion of the document is visible in the main editor
- **Click/drag navigation** — clicking in the map scrolls the main editor to that position
- **Content sync** — connect to editor's `textChanged` signal (debounced, 500ms)
- Toggle via View > Document Map

**New files:** `src/ui/documentmap.h`, `src/ui/documentmap.cpp`
**Files modified:** `src/ui/mainwindow.h`, `src/ui/mainwindow.cpp`, `CMakeLists.txt`

### 3.2 Function List

A left or right side panel showing a tree of functions/classes/methods in the current document.

**Implementation:**
- `FunctionListPanel` — a `QWidget` containing a `QTreeView` with `QStandardItemModel`
- **Parsing:** use the `functionRegex` field from the language's syntax definition JSON to extract function signatures
- **Tree structure:** for languages with classes, show class → methods hierarchy. For flat languages, show a flat list.
- **Navigation:** double-click a function to jump to its line
- **Refresh:** debounced on text change (1s delay), also refresh on tab switch
- **Sort option:** alphabetical or order of appearance
- Toggle via View > Function List

**New files:** `src/ui/functionlist.h`, `src/ui/functionlist.cpp`
**Files modified:** `src/ui/mainwindow.h`, `src/ui/mainwindow.cpp`, `CMakeLists.txt`

### 3.3 Auto-Completion

Extend the existing `CompletionPopup` class with proper completion sources.

**Sources (in priority order):**
1. **Language keywords** — from the active language's syntax definition (keywords + types + builtins)
2. **Document words** — all unique words (4+ chars) from the current document
3. **Open document words** — words from all other open documents (lower priority)

**Trigger:**
- **Manual:** Ctrl+Space
- **Automatic:** after typing 3+ consecutive word characters (configurable, can be disabled)

**Behavior:**
- Popup appears below cursor with filtered list
- Fuzzy matching (not just prefix)
- Enter/Tab to accept, Escape to dismiss
- Arrow keys to navigate, typing continues to filter

**Files modified:** `src/ui/completionpopup.h`, `src/ui/completionpopup.cpp`, `src/ui/editor.cpp` (trigger logic)

### 3.4 Macro Run N Times

Add "Run Macro Multiple Times" dialog:

```
Run Current Macro:
  ( ) Run [____] times
  ( ) Run until end of file

[Run] [Cancel]
```

"Run until end of file" — playback macro repeatedly, advancing cursor, until cursor reaches the last line.

**New file:** `src/ui/macrodialog.h`, `src/ui/macrodialog.cpp`
**Files modified:** `src/ui/mainwindow.h`, `src/ui/mainwindow.cpp` (add menu item and slot), `CMakeLists.txt`

### 3.5 Plugin Menu + Startup Loading

Wire the existing PluginManager into the application:

1. **Startup loading** — in `main.cpp`, after creating MainWindow, call `PluginManager::instance().loadPlugins()`. Default scan directory: `~/.config/OliveNotepad/plugins/` and `/usr/lib/olive-notepad/plugins/`.

2. **Plugins menu** — add to menu bar between Run and Window:
   - List of loaded plugins with checkbox to enable/disable
   - "Open Plugin Directory..." — opens the plugin directory in file manager
   - Plugin commands appear as sub-items under each plugin

3. **Plugin event wiring** — connect DocumentManager signals to PluginManager broadcast methods (fileOpened, fileSaved, fileClosed). Connect Editor signals to textChanged/cursorMoved broadcasts.

**Files modified:** `src/main.cpp`, `src/ui/mainwindow.h`, `src/ui/mainwindow.cpp`

---

## Files Summary

### New Files (14+)
| File | Purpose |
|------|---------|
| `src/core/charsetdetector.h` | Heuristic charset detector |
| `src/core/charsetdetector.cpp` | Byte-frequency encoding detection |
| `src/ui/notificationbar.h` | File-changed notification bar |
| `src/ui/notificationbar.cpp` | Notification bar implementation |
| `src/ui/recoverydialog.h` | Startup recovery dialog |
| `src/ui/recoverydialog.cpp` | Recovery dialog implementation |
| `src/ui/documentmap.h` | Document map minimap widget |
| `src/ui/documentmap.cpp` | Document map implementation |
| `src/ui/functionlist.h` | Function list side panel |
| `src/ui/functionlist.cpp` | Function list implementation |
| `src/ui/macrodialog.h` | Macro run-N-times dialog |
| `src/ui/macrodialog.cpp` | Macro dialog implementation |
| `resources/syntax/*.json` | 80+ language syntax definitions |

### Modified Files
| File | Changes |
|------|---------|
| `src/syntax/highlighter.h` | Remove 13 loadXxxRules, add loadFromDefinition |
| `src/syntax/highlighter.cpp` | Data-driven syntax loader, theme-aware colors |
| `src/syntax/languagemanager.h` | Add definitionForLanguage accessor |
| `src/syntax/languagemanager.cpp` | Register JSON definitions, improved auto-detection |
| `src/core/encoding.cpp` | Full encoding support (CJK, ISO-8859, KOI8) |
| `src/core/document.cpp` | Use charset detector, persistent unsaved support |
| `src/core/documentmanager.h` | QFileSystemWatcher member, file change signals |
| `src/core/documentmanager.cpp` | File monitoring, watcher management |
| `src/core/session.h` | Unsaved document persistence methods |
| `src/core/session.cpp` | Save/restore unsaved documents, content caching |
| `src/ui/mainwindow.h` | New dock widgets, menu pointers, slots |
| `src/ui/mainwindow.cpp` | Document map, function list, plugin menu, reload, modified closeEvent |
| `src/ui/completionpopup.h` | Multi-source completion, fuzzy matching |
| `src/ui/completionpopup.cpp` | Completion sources, trigger logic |
| `src/ui/editor.cpp` | Auto-completion trigger, completion integration |
| `src/main.cpp` | Recovery dialog, plugin loading on startup |
| `CMakeLists.txt` | Add new source files |
| `resources/resources.qrc` | Add syntax JSON files |

---

## What Is NOT Being Built (Phase 2)

- TextMate grammar (.tmLanguage) support — JSON definitions are sufficient and simpler
- LSP integration — deferred to Phase 3
- Git integration panels — deferred to Phase 3
- Remote file connections — deferred to Phase 3
- Multi-cursor (Ctrl+click) — deferred
- Snippet system — auto-completion covers keyword/word completion; snippets are Phase 3
- Style Configurator dialog — stub remains from Phase 1
- Shortcut Mapper dialog — stub remains from Phase 1

---

## Build Sequence

1. **Syntax definition format + generic loader** — convert C++ as first language, validate
2. **Convert 13 existing languages to JSON** — remove all hardcoded methods
3. **Add 70+ language definitions** — batch creation of JSON files
4. **Theme-aware syntax colors** — connect highlighter to ThemeManager
5. **Charset detector + encoding hardening** — new detection, full encoding support
6. **File change monitoring** — QFileSystemWatcher + notification bar
7. **Persistent unsaved documents** — session save/restore for unsaved content
8. **Recovery dialog** — startup recovery prompt
9. **Reload from disk** — menu action
10. **Document map** — minimap dock widget
11. **Function list** — side panel with function extraction
12. **Auto-completion** — extend CompletionPopup with sources and triggers
13. **Macro run N times** — dialog and execution loop
14. **Plugin menu + startup loading** — wire PluginManager to UI
15. **Print hardening** — print preview, headers/footers
16. **Integration verification** — full test pass

Each step produces a compilable, runnable binary.

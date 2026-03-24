# Notepad Supreme: Requirements Specification

**Project:** Native Linux Text Editor  
**Version:** 1.0  
**Target Platform:** Linux (Ubuntu primary, cross-distribution support)  
**License:** Open Source (GPL or MIT)  
**Recommended Stack:** C++ with Qt or GTK toolkit

---

## 1. Project Overview

Notepad Supreme is a native Linux text editor designed to match and exceed Notepad++ functionality while incorporating modern features from VS Code and Sublime Text. The application must be lightweight, fast, and deeply integrated with the Linux desktop environment.

### 1.1 Core Objectives

- Full feature parity with Notepad++
- Native Linux platform integration (GNOME/KDE)
- Lightweight resource consumption
- Extensible plugin architecture
- Modern developer-focused features (LSP, integrated terminal, Git)

---

## 2. File and Editing Fundamentals

### 2.1 Performance Requirements

| Requirement | Specification |
|-------------|---------------|
| Startup Time | < 1 second on standard hardware |
| Large File Handling | Open and scroll files > 1GB without degradation |
| Memory Footprint | < 100MB with 10 files open |
| Scroll Performance | 60 FPS smooth scrolling on large files |

### 2.2 Tabbed Interface

- Multiple documents in single window with tab bar
- Drag-and-drop tab reordering
- Close button on each tab
- Middle-click to close tabs
- Right-click context menu (Close, Close Others, Close All, Close to Right)
- Tab overflow handling with scroll arrows or dropdown
- Modified indicator (asterisk or dot) on unsaved tabs
- Tab preview on hover (optional setting)

### 2.3 Multi-View and Split-Screen

- Horizontal split (one above other)
- Vertical split (side by side)
- Arbitrary split combinations (quad view, etc.)
- Synchronized scrolling option for split views
- Clone document into new split (same file, different view)
- Independent cursor positions per split

### 2.4 File Encoding Support

| Feature | Requirements |
|---------|--------------|
| Encodings | UTF-8, UTF-8 BOM, UTF-16 LE/BE, UTF-32, ASCII, ISO-8859-1 through ISO-8859-15, Windows-1252, KOI8-R, Shift-JIS, GB2312, Big5 |
| Line Endings | Unix (LF), Windows (CRLF), Classic Mac (CR) |
| Conversion | Convert between any encoding/line ending on save |
| Detection | Auto-detect encoding on file open |
| Status Bar | Display current encoding and line ending |

### 2.5 Basic Operations

- Cut, Copy, Paste with system clipboard integration
- Unlimited Undo/Redo history (persisted per session)
- Drag-and-drop file opening from file manager
- Drag-and-drop text within and between documents
- Recent files list
- Auto-save to temporary location (configurable interval)

### 2.6 Printing

- Print preview
- Customizable headers and footers (filename, date, page numbers)
- Margin configuration (top, bottom, left, right)
- Line number printing option
- Syntax highlighting in print output
- Page range selection

---

## 3. Programming and Code Features

### 3.1 Syntax Highlighting

**Minimum Required Languages (100+):**

| Category | Languages |
|----------|-----------|
| Systems | C, C++, Rust, Go, Assembly |
| Web | HTML, CSS, JavaScript, TypeScript, PHP |
| Scripting | Python, Ruby, Perl, Bash, PowerShell, Lua |
| Data | JSON, YAML, XML, TOML, INI, CSV |
| Markup | Markdown, LaTeX, reStructuredText |
| Database | SQL, PostgreSQL, MySQL |
| Config | Nginx, Apache, Dockerfile, Makefile |
| JVM | Java, Kotlin, Scala, Groovy |
| .NET | C#, F#, VB.NET |
| Functional | Haskell, OCaml, Erlang, Elixir |
| Mobile | Swift, Objective-C, Dart |

**Implementation Requirements:**
- TextMate grammar support (.tmLanguage)
- User-customizable color schemes
- Per-language color overrides
- High-contrast accessibility themes

### 3.2 Code Folding

- Function/method folding
- Class/struct folding
- Loop and conditional block folding
- Region markers (#region / #endregion or equivalent)
- Fold all / Unfold all commands
- Fold level controls (fold to level 1, 2, 3, etc.)
- Persistent fold state across sessions
- Gutter fold indicators (clickable +/- icons)

### 3.3 Auto-Completion

- Keyword completion for current language
- Standard library function suggestions
- User-defined word completion (words from open documents)
- Snippet expansion
- Path completion for file paths
- Fuzzy matching support
- Completion popup with documentation preview

### 3.4 Bracket and Indentation Matching

- Highlight matching brackets: (), [], {}, <>
- Jump to matching bracket (keyboard shortcut)
- Rainbow bracket coloring (optional)
- Smart auto-indentation based on language
- Indent guides (vertical lines showing indent level)
- Auto-close brackets option (configurable per language)
- Tab/space indent toggle (per file or global)
- Configurable tab width (2, 4, 8 spaces)

### 3.5 Macros

- Record macro (start/stop toggle)
- Play macro (single execution)
- Play macro N times
- Save macro to file
- Load macro from file
- Assign macro to keyboard shortcut
- Macro includes: keystrokes, search/replace, cursor movement

---

## 4. Search and Manipulation

### 4.1 Find and Replace

**Scope Options:**
- Current selection
- Current file
- All open files
- Directory with file filters (*.py, *.js, etc.)
- Project/workspace scope

**Search Modes:**
- Literal text
- Regular expressions (PCRE2 or RE2)
- Whole word only
- Case sensitive / insensitive

**Features:**
- Incremental search (highlight as you type)
- Find all with results panel
- Replace all with preview
- Multi-line search and replace
- Search history (recent searches)
- Bookmarked search patterns

### 4.2 Multi-Cursor Editing

- Alt + Click to add cursor
- Alt + Drag for column/box selection
- Ctrl + D to select next occurrence
- Ctrl + Shift + L to select all occurrences
- Independent cursor undo
- Cursors persist across operations
- Clear all cursors with Escape

### 4.3 Column/Block Editing

- Rectangular selection with Alt + Drag
- Column insert mode
- Column paste (paste lines across rows)
- Fill column with text
- Number column sequentially

### 4.4 Line Operations

| Operation | Description |
|-----------|-------------|
| Duplicate Line | Copy current line below |
| Move Line Up/Down | Reorder lines with keyboard |
| Delete Line | Remove entire line |
| Join Lines | Merge selected lines into one |
| Sort Lines | Alphabetical, numerical, reverse |
| Case Conversion | UPPER, lower, Title Case, camelCase, snake_case, kebab-case |
| Remove Duplicates | Delete duplicate lines |
| Remove Empty Lines | Delete blank lines |
| Trim Whitespace | Leading, trailing, or both |

---

## 5. Customization and Extensibility

### 5.1 Theming

- Light and dark mode with system preference detection
- Full UI theme customization (not just syntax colors)
- Import/export themes
- Theme editor with live preview
- Minimum included themes: Default Light, Default Dark, Solarized Light, Solarized Dark, Monokai, Dracula, Nord

### 5.2 Plugin Architecture

**API Requirements:**
- Well-documented C++ or scripting API (Lua or Python bindings)
- Event hooks: file open, save, close, edit, selection change
- Access to editor buffer, cursor, selection
- Custom menu items and toolbar buttons
- Custom panels and dialogs
- Settings integration
- Inter-plugin communication

**Plugin Management:**
- Built-in plugin manager UI
- Install from repository
- Enable/disable plugins
- Plugin settings panel
- Dependency resolution

---

## 6. Advanced Code Intelligence

### 6.1 Integrated Terminal

- Embedded terminal panel (bash, zsh, fish support)
- Multiple terminal tabs
- Configurable shell
- Send selected text to terminal
- Terminal split view
- Copy/paste support
- Color theme matching editor theme

### 6.2 Language Server Protocol (LSP)

**Features:**
- Auto-start language servers for supported languages
- Intelligent code completion (IntelliSense-style)
- Hover information (type info, documentation)
- Go to definition
- Go to references
- Find all references
- Rename symbol
- Code actions (quick fixes, refactoring)
- Real-time diagnostics (errors, warnings)
- Signature help

**Configuration:**
- User-configurable language server paths
- Per-language server settings
- Disable LSP per language option

### 6.3 Project and Workspace Management

- File explorer panel (tree view)
- Open folder as project
- Project-specific settings (.notepad-supreme folder or similar)
- File exclusion patterns (node_modules, .git, etc.)
- Quick file open (fuzzy search by filename)
- Quick symbol search (functions, classes across project)
- Workspace concept (multiple root folders)

### 6.4 Git Integration

**File Explorer Indicators:**
- Modified files (yellow/orange)
- Staged files (green)
- Untracked files (gray/dim)
- Conflicted files (red)

**Gutter Indicators:**
- Added lines (green bar)
- Modified lines (blue bar)
- Deleted lines (red triangle)

**Features:**
- Inline diff view
- Stage/unstage hunks
- Commit with message dialog
- Branch display in status bar
- Branch switching
- Basic merge conflict resolution UI

### 6.5 Remote File Editing

- SSH/SFTP connection manager
- Save connections with credentials (secure storage)
- Browse remote directories
- Edit remote files (automatic sync on save)
- FTP support (optional, lower priority)
- Connection status indicator

---

## 7. Enhanced User Experience

### 7.1 Command Palette

- Unified command search (Ctrl+Shift+P or similar)
- Fuzzy search all commands
- Recently used commands
- Keyboard shortcut display next to commands
- Quick settings access
- Extension commands integration

### 7.2 Go To Anything

- Quick file open (Ctrl+P): fuzzy filename search
- Go to line (:line_number)
- Go to symbol (@symbol_name)
- Combined search (file@symbol:line)
- Recent files prioritized

### 7.3 Session Management

**Saved State:**
- All open files (with paths)
- Cursor positions per file
- Scroll positions per file
- Split view configuration
- Active tab
- Fold states
- Project/folder context

**Features:**
- Auto-restore on startup (configurable)
- Named sessions (save, load, delete)
- Session file location configurable

### 7.4 Markdown Support

- Live preview panel (side-by-side or popup)
- GitHub-flavored Markdown support
- Syntax highlighting in code blocks
- Table formatting assistance
- Image preview inline
- Export to HTML
- Export to PDF (via system tools)

### 7.5 Hex/Binary Viewer

- Toggle hex view mode
- Byte offset display
- ASCII representation alongside hex
- Go to offset
- Search by hex pattern
- Editable hex mode (with caution warnings)
- File size and encoding info

### 7.6 AI Text Utilities (Future Enhancement)

- Local/offline AI integration preferred
- Rephrase selected text
- Explain code block
- Generate docstring from function
- Translate text
- Summarize selection
- Configurable AI backend (optional cloud APIs)

---

## 8. Smart Language Detection

### 8.1 Automatic Identification

**Detection Methods:**
- File extension mapping (primary)
- Shebang line parsing (#!/usr/bin/python, etc.)
- Content heuristics for new/unsaved files

**Content Heuristics Examples:**
| Pattern | Detected Language |
|---------|-------------------|
| `<!DOCTYPE html>` or `<html` | HTML |
| `<?php` | PHP |
| `package main` + `func` | Go |
| `class X {` + `public static void main` | Java |
| `def ` + `:` (indentation-based) | Python |
| `{ "key":` structure | JSON |
| `key: value` structure | YAML |

### 8.2 Implementation Requirements

- Detection runs asynchronously (non-blocking)
- Apply syntax highlighting immediately upon detection
- Confidence threshold before auto-applying
- User can override detected language
- Status bar shows detected language (clickable to change)
- Detection on file open and first N characters typed

---

## 9. Session Recovery and Crash Protection

### 9.1 Automatic Backup

**Backup Frequency:** Every 5-10 seconds (configurable)

**Backup Location:** 
- `~/.local/share/notepad-supreme/recovery/` or XDG-compliant path
- Hidden from user unless recovery needed

**Backed Up Data:**
- Full content of unsaved files
- File path (if previously saved)
- Cursor position
- Scroll position
- Language mode
- Fold state
- Undo history (optional, may be large)

### 9.2 Recovery Process

**On Startup After Crash:**
1. Detect abnormal previous shutdown (lock file or flag)
2. Display recovery dialog listing recoverable files
3. Options: Restore All, Restore Selected, Discard All
4. Restore files to exact previous state
5. Mark restored files as modified (require explicit save)

**Normal Shutdown:**
- Clean up all recovery files
- Save session state normally

---

## 10. Linux Platform Integration

### 10.1 Desktop Environment

- Follow XDG Base Directory Specification for config and data
- System theme integration (GTK or Qt theme matching)
- System font preferences respected
- Desktop notifications for background events
- D-Bus integration for IPC
- File association registration (.txt, .py, .js, etc.)
- "Open with" context menu integration

### 10.2 Package Distribution

**Required Formats:**
- AppImage (universal, primary distribution)
- Flatpak (sandboxed, Flathub listing)
- Snap (Ubuntu Software Center)
- .deb (Debian, Ubuntu, Mint)
- .rpm (Fedora, RHEL, openSUSE)
- AUR PKGBUILD (Arch Linux)

### 10.3 Resource Efficiency

| Metric | Target |
|--------|--------|
| Cold Start RAM | < 50MB |
| Per-Tab Overhead | < 5MB for typical files |
| CPU at Idle | < 1% |
| Disk I/O | Minimal (lazy loading, efficient caching) |

---

## 11. Technical Requirements

### 11.1 Development Stack

| Component | Recommendation |
|-----------|----------------|
| Language | C++ (C++17 or later) |
| UI Toolkit | Qt 6 (preferred) or GTK 4 |
| Build System | CMake |
| Text Engine | Custom rope-based or piece table for large files |
| Syntax Engine | Tree-sitter or TextMate grammars |
| LSP | Built-in LSP client library |
| Terminal | libvterm or similar |

### 11.2 Testing Requirements

- Unit tests for core editing operations
- Integration tests for file I/O
- UI automation tests for critical workflows
- Performance benchmarks (startup, large file, scrolling)
- Memory leak detection in CI
- Cross-distribution testing (Ubuntu, Fedora, Arch minimum)

### 11.3 Accessibility

- Screen reader compatibility (AT-SPI2)
- High contrast theme included
- Keyboard-only navigation for all features
- Configurable font sizes
- Focus indicators

---

## 12. Implementation Priorities

### Phase 1: Core Editor (MVP)
1. Basic file open/save/edit with tabs
2. Syntax highlighting (20 major languages)
3. Find and replace (current file)
4. Encoding and line ending handling
5. Undo/redo
6. Basic preferences dialog

### Phase 2: Power Features
7. Code folding
8. Multi-cursor editing
9. Column selection
10. Project file explorer
11. Find in files
12. Session restore and crash recovery
13. Smart language detection

### Phase 3: Developer Tools
14. Integrated terminal
15. LSP integration
16. Git status indicators
17. Command palette
18. Macro recording

### Phase 4: Ecosystem
19. Plugin architecture
20. Theme editor
21. Markdown preview
22. Hex viewer
23. Remote file editing
24. Full Git integration

---

## 13. Success Criteria

The editor is considered complete when:

1. All Phase 1-3 features are implemented and stable
2. Opens 1GB file in < 5 seconds
3. Startup time < 1 second
4. Memory usage < 100MB with 10 typical files
5. Zero data loss in crash recovery tests
6. Passes accessibility audit
7. Available in at least 3 package formats
8. Plugin API documented with example plugins
9. 80%+ unit test coverage on core modules
10. Community-validated usability (beta testing)

---

## Appendix A: Keyboard Shortcuts Reference

| Action | Shortcut |
|--------|----------|
| New File | Ctrl+N |
| Open File | Ctrl+O |
| Save | Ctrl+S |
| Save As | Ctrl+Shift+S |
| Close Tab | Ctrl+W |
| Undo | Ctrl+Z |
| Redo | Ctrl+Y or Ctrl+Shift+Z |
| Find | Ctrl+F |
| Replace | Ctrl+H |
| Find in Files | Ctrl+Shift+F |
| Go to Line | Ctrl+G |
| Quick Open File | Ctrl+P |
| Command Palette | Ctrl+Shift+P |
| Toggle Terminal | Ctrl+` |
| New Terminal | Ctrl+Shift+` |
| Split Vertical | Ctrl+\ |
| Split Horizontal | Ctrl+Shift+\ |
| Toggle Sidebar | Ctrl+B |
| Go to Definition | F12 |
| Peek Definition | Alt+F12 |
| Find References | Shift+F12 |
| Rename Symbol | F2 |
| Toggle Comment | Ctrl+/ |
| Duplicate Line | Ctrl+Shift+D |
| Move Line Up | Alt+Up |
| Move Line Down | Alt+Down |
| Add Cursor Below | Ctrl+Alt+Down |
| Add Cursor Above | Ctrl+Alt+Up |
| Select All Occurrences | Ctrl+Shift+L |
| Select Next Occurrence | Ctrl+D |

---

## Appendix B: Configuration File Format

```json
{
  "editor": {
    "fontFamily": "JetBrains Mono, Fira Code, monospace",
    "fontSize": 14,
    "tabSize": 4,
    "insertSpaces": true,
    "wordWrap": "off",
    "lineNumbers": true,
    "minimap": true,
    "renderWhitespace": "selection",
    "autoSave": "afterDelay",
    "autoSaveDelay": 1000
  },
  "theme": {
    "name": "Default Dark",
    "customColors": {}
  },
  "terminal": {
    "shell": "/bin/bash",
    "fontSize": 13
  },
  "files": {
    "encoding": "utf8",
    "eol": "\n",
    "trimTrailingWhitespace": true,
    "insertFinalNewline": true
  }
}
```

---

*End of Requirements Specification*

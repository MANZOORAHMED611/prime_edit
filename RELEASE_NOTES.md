# PrimeEdit 1.0 — Release Notes

**Released:** March 2026  
**Platform:** Linux (x86_64)  
**Build:** Qt 6.2.4 | C++17 | Zero build errors

---

## This Release

PrimeEdit 1.0 is the first public release of a native Linux text editor built to match and exceed Notepad++ in every meaningful capability.

It is not a port. It is not a wrapper. It is a ground-up C++ application built for Linux — fast, native, and feature-complete.

---

## What's Included

### Core Editor
A PieceTable-backed editor with unlimited undo/redo, multi-cursor editing, split-screen, column selection, code folding (brace-based and indent-based), bookmark system, whitespace visualization, indent guides, rainbow bracket coloring, and a three-column gutter.

### Large File Handling
Three-mode architecture that handles files other editors refuse to open. A 93MB single-line minified JSON file — the case that hangs VS Code, gedit, and challenges Notepad++ — opens in PrimeEdit instantly at approximately 1MB RSS. Files over 50MB use chunked lazy loading with minified file detection.

### LSP Integration
A full JSON-RPC language server client. Go-to-definition, find references, rename symbol, hover documentation, inline diagnostics, and LSP-powered completions. Configure any language server via a single JSON file.

### Search System
Tabbed search dialog with Find, Replace, Find in Files, and Mark. Normal, Extended, and Regex modes. Incremental search, 20-entry history, dockable results panel, dialog transparency slider.

### Git Integration
Gutter indicators for added, modified, and deleted lines. Commit dialog with staging. Branch switching. Status bar branch display.

### Multi-Cursor Editing
Alt+Click, Ctrl+D for next match, Ctrl+Shift+L for all occurrences. Simultaneous typing, deletion, and bracket insertion across all cursors with atomic undo.

### Split-Screen Editing
Vertical and horizontal splits. Clone or move documents between panes. F6 focus switching.

### Syntax Highlighting
96 languages via JSON definitions. Data-driven — adding a language requires no C++ changes. Auto-detection by extension, shebang, and content heuristics. Theme-aware colors.

### Plugin System
C++ shared library API with full EditorAPI surface. Event hooks for all file, text, and cursor events. Plugin management dialog. Crash isolation — a failing plugin cannot take down the editor.

### Remote File Editing
SSH/SCP-based remote file browser with auto-upload on save. Strict host key verification enabled by default.

### Session & Recovery
Full session restore across restarts. Crash recovery with configurable auto-save. Unsaved documents persist across application close.

### Themes
9 built-in themes including Notepad++ Classic, Monokai, Dracula, Nord, and Solarized variants. Theme editor with JSON import/export.

### AI-Assisted Editing
Select any text, press Ctrl+Shift+E, and send it to any LLM endpoint — local Ollama or any OpenAI-compatible API. Inline diff shows exactly what changed. Accept with A, discard with Escape. Full undo support.

---

## Validation

PrimeEdit 1.0 shipped after three rounds of validation:

**Feature validation:** 148 tests covering every documented capability — PieceTable operations, encoding, search, syntax highlighting, large file handling, settings, themes, macros, and resource integrity. All 148 passed.

**Stress testing:** 70 crash and stress tests including 1,000 undo/redo cycles, 50 rapid tab open/close operations, all 9 themes switched 50 times with multiple editors open, 1MB single-line content, mixed Unicode (Arabic, Chinese, Japanese, Korean, emoji), invalid regex patterns, concurrent edit during search, and theme switching during text modification. Zero crashes.

**Bug resolution:** 79 issues identified and resolved across multiple QA passes — 13 critical, 28 high, 21 medium, 17 low. Zero known critical issues at release.

---

## Performance Benchmarks

| Scenario | Result |
|----------|--------|
| 93MB minified JSON (1 line) | Opens instantly, ~1MB RSS |
| 50MB multi-line log file | Opens in <2 seconds, ~8MB RSS |
| 1GB file memory target | <200MB RSS (sparse line index) |
| Syntax highlight response | Viewport-only, no UI block |
| Regex search on large files | Background thread, progressive results |

---

## Known Limitations

- **Large file editing:** Files over 50MB open in read-only preview mode. Full editing support for large files is planned for a future release.
- **Plugin marketplace:** The plugin API is complete and documented. A plugin registry is not yet available.
- **Windows/macOS:** Linux only in this release. Cross-platform support is not currently planned.
- **Extended encoding:** Windows-1252 and ISO-8859 family detection may fall back to UTF-8 on ambiguous files.

---

## Compared to Notepad++

| Feature | Notepad++ | PrimeEdit 1.0 |
|---------|-----------|----------------|
| Linux native | ❌ (Wine only) | ✅ |
| Multi-cursor | ❌ | ✅ |
| Split-screen | ❌ | ✅ |
| LSP integration | ❌ | ✅ |
| Git gutter | ❌ | ✅ |
| Remote file editing | ❌ | ✅ |
| AI-assisted editing | ❌ | ✅ |
| Large file handling | Partial | ✅ |
| Syntax languages | 80+ | 96 |
| Plugin system | ✅ | ✅ |
| Macro system | ✅ | ✅ |
| Themes | ✅ | ✅ (9 built-in) |

---

## Installation

**AppImage:**
```bash
chmod +x PrimeEdit-1.0-x86_64.AppImage
./PrimeEdit-1.0-x86_64.AppImage
```

**Debian/Ubuntu:**
```bash
sudo dpkg -i primeedit_1.0_amd64.deb
```

**Build from source:**
```bash
git clone https://github.com/MANZOORAHMED611/prime_edit.git
cd prime_edit
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

---

## What's Next

- Large file editing (not just viewing)
- Plugin registry and first-party plugins
- Extended encoding support (CJK, full ISO-8859 family)
- Accessibility (AT-SPI2 screen reader support)
- Additional LSP features (code actions, auto-fix)

---

*Green Olive Tech LLC — greenolivetech.com*  
*PrimeEdit is MIT licensed. Contributions welcome.*

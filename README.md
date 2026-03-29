# PrimeEdit

**A fast, native text editor for Linux — built to rival and exceed Notepad++.**

PrimeEdit is a production-grade C++ editor built on Qt 6, designed for fast, reliable text editing on Linux.

It handles everything from quick edits to large log files, while providing modern capabilities like multi-cursor editing, split views, and optional language server integration — all in a lightweight native application.

---

## Why PrimeEdit

Linux has always lacked a native equivalent to Notepad++. VS Code is an Electron IDE. gedit is a basic notepad. Kate is KDE-specific. Notepad++ itself doesn't run natively on Linux.

PrimeEdit fills that gap — and goes further.

---

## Features

### Editor Core
- PieceTable text buffer with unlimited undo/redo
- Line numbers, current line highlight, three-column gutter
- Bracket matching with jump (Ctrl+B) and rainbow coloring
- Code folding — brace-based (C/C++/Java/JS) and indent-based (Python/YAML)
- Column/block selection (Alt+drag) and Column Editor
- Whitespace visualization, EOL markers, indent guides
- Bookmark system (Ctrl+F2 toggle, Ctrl+Shift+F2 navigate)
- Word wrap, zoom in/out, distraction-free mode

### Multi-Cursor Editing
- Alt+Click to place cursors, Ctrl+D to select next match
- Ctrl+Shift+L to select all occurrences
- Simultaneous typing, deletion, bracket insertion, atomic undo

### Split-Screen Editing
- Vertical and horizontal splits (View menu)
- Clone or move documents between splits
- F6 to switch focus between panes

### Large File Handling
- Three-mode architecture: Small (<20MB), Medium (20–50MB), Large (>50MB)
- Memory-mapped I/O with viewport loading — only visible lines in memory
- Minified file detection — 93MB single-line JSON opens instantly at ~1MB RSS
- Chunked lazy loading for files over 50MB
- Syntax highlighting disabled above 5MB for performance

### Search System
- Find / Replace / Find in Files / Mark — tabbed dialog
- Normal, Extended (`\n \t \xNN`), and Regex modes
- Incremental search (Ctrl+I), search history (20 entries)
- Find All in Current Document and All Open Documents
- Results panel — dockable, grouped by file, click-to-navigate
- Dialog transparency slider

### Syntax Highlighting
- 96 languages via JSON definitions — data-driven, no hardcoded rules
- Auto-detection by extension, shebang, and content heuristics
- Theme-aware colors, live update on theme change

### LSP Integration
- JSON-RPC language server client (stdio/TCP)
- Go-to-definition (F12), find references (Shift+F12), rename symbol (F2)
- Hover documentation, inline diagnostics (wave underlines)
- LSP-powered completions replace word-list completions when a server is active
- Per-language server configuration via `~/.config/PrimeEdit/lsp-servers.json`

### Git Integration
- Gutter indicators: green (added), yellow (modified), red (deleted)
- Commit dialog with staging, branch switching
- Status bar shows current branch
- File change monitoring with reload prompt

### Auto-Completion
- 300ms debounced trigger, fuzzy matching
- Keyboard navigation (Up/Down/Tab/Enter/Escape)
- File path completion, snippet expansion (7 languages)

### Plugin System
- C++ shared library API with full EditorAPI (buffer, file, UI, settings)
- Event hooks: file opened/saved/closed, text changed, cursor moved
- Plugin management dialog, crash isolation per plugin
- Plugin directories: `~/.config/PrimeEdit/plugins/`

### Remote File Editing
- SSH/SCP-based connection manager
- Remote file browser, auto-upload on save
- Strict host key checking (secure default)

### Session & Macros
- Session save/restore — open files, cursor positions, fold state, bookmarks
- Crash recovery with auto-save (configurable interval)
- Persistent unsaved documents survive app close
- Macro record/stop/playback (F9/Shift+F9/F10), save/load to file

### UI & Themes
- 9 built-in themes: Notepad++ Classic, Default Light, Default Dark, Olive, Monokai, Dracula, Nord, Solarized Light, Solarized Dark
- Theme editor and import/export (JSON)
- Enhanced tab bar: file-type icons, close-on-hover, modified indicator
- Document map minimap (right dock)
- Function list panel (left dock) — 9 languages
- Integrated terminal (Ctrl+')
- File explorer panel
- Full printing with line numbers, headers/footers, syntax-highlighted output
- Command palette (Ctrl+Shift+P)

### Encoding & File Handling
- UTF-8, UTF-8-BOM, UTF-16 LE/BE, Latin-1, Windows-1252, ISO-8859
- Heuristic charset detection (BOM, UTF-8 validation, byte-frequency)
- Line ending detection and conversion (LF, CRLF, CR)
- File change monitoring (external modification prompt)

---

## Installation

### AppImage (Recommended)
```bash
chmod +x PrimeEdit-1.0-x86_64.AppImage
./PrimeEdit-1.0-x86_64.AppImage
```

### .deb (Debian/Ubuntu)
```bash
sudo dpkg -i primeedit_1.0_amd64.deb
```

### Build from Source
**Requirements:** Qt 6.2+, CMake 3.16+, C++17 compiler

```bash
git clone https://github.com/MANZOORAHMED611/prime_edit.git
cd prime_edit
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./PrimeEdit
```

---

## LSP Setup

Create `~/.config/PrimeEdit/lsp-servers.json`:

```json
{
  "cpp": {
    "command": "clangd",
    "args": []
  },
  "python": {
    "command": "pylsp",
    "args": []
  },
  "rust": {
    "command": "rust-analyzer",
    "args": []
  }
}
```

---

## AI-Assisted Editing (Optional)

Select any text and press **Ctrl+Shift+E** to send it to a configured LLM endpoint. An inline diff shows what changed — press **A** to accept or **Escape** to discard. Works with Ollama (local) or any OpenAI-compatible endpoint.

---

## Keyboard Shortcuts

### File

| Action | Shortcut |
|--------|----------|
| New File | Ctrl+N |
| Open File | Ctrl+O |
| Open Remote | Ctrl+Shift+O |
| Save | Ctrl+S |
| Save As | Ctrl+Alt+S |
| Save All | Ctrl+Shift+S |
| Reload from Disk | Ctrl+Shift+R |
| Close | Ctrl+W |
| Print | Ctrl+P |

### Edit

| Action | Shortcut |
|--------|----------|
| Undo | Ctrl+Z |
| Redo | Ctrl+Y |
| Cut / Copy / Paste | Ctrl+X / C / V |
| Select All | Ctrl+A |
| Duplicate Line | Ctrl+Shift+D |
| Delete Line | Ctrl+Shift+K |
| Move Line Up/Down | Alt+Up/Down |
| Toggle Comment | Ctrl+/ |
| Column Editor | Alt+C |

### Multi-Cursor

| Action | Shortcut |
|--------|----------|
| Add Cursor | Alt+Click |
| Select Next Occurrence | Ctrl+D |
| Select All Occurrences | Ctrl+Shift+L |
| Cancel Multi-Cursor | Escape |

### Search & Navigation

| Action | Shortcut |
|--------|----------|
| Find | Ctrl+F |
| Replace | Ctrl+H |
| Find Next / Previous | F3 / Shift+F3 |
| Find in Files | Ctrl+Shift+F |
| Incremental Search | Ctrl+I |
| Go to Line | Ctrl+G |
| Jump to Bracket | Ctrl+B |
| Go to Definition | F12 |
| Find References | Shift+F12 |
| Rename Symbol | F2 |

### Bookmarks

| Action | Shortcut |
|--------|----------|
| Toggle Bookmark | Ctrl+F2 |
| Next Bookmark | Ctrl+Shift+F2 |
| Previous Bookmark | Ctrl+Alt+F2 |

### View

| Action | Shortcut |
|--------|----------|
| Split Vertical | Ctrl+\\ |
| Split Horizontal | Ctrl+Shift+\\ |
| Close Split | Ctrl+Alt+\\ |
| Focus Other Split | F6 |
| Next Tab | Ctrl+PageDown |
| Previous Tab | Ctrl+PageUp |
| Terminal | Ctrl+' |
| Command Palette | Ctrl+Shift+P |
| Full Screen | F11 |
| Distraction-Free | Ctrl+Shift+F11 |
| Zoom In / Out / Reset | Ctrl++ / Ctrl+- / Ctrl+0 |

### Macros & Git

| Action | Shortcut |
|--------|----------|
| Record Macro | F9 |
| Stop Recording | Shift+F9 |
| Play Macro | F10 |
| Git Commit | Ctrl+Shift+G |
| AI Evaluate Selection | Ctrl+Shift+E |

---

## Configuration Files

| File | Location | Purpose |
|------|----------|---------|
| LSP servers | `~/.config/PrimeEdit/lsp-servers.json` | Language server commands |
| Eval endpoint | `~/.config/PrimeEdit/eval-endpoint.json` | LLM endpoint for AI editing |
| Session data | `~/.local/share/PrimeEdit/sessions/` | Open tabs, cursor positions |
| Custom themes | `~/.local/share/PrimeEdit/themes/` | JSON theme files |
| Plugins | `~/.config/PrimeEdit/plugins/` | Plugin shared libraries |

---

## Performance

| File | Memory | Open Time |
|------|--------|-----------|
| 93MB minified JSON (1 line) | ~1MB RSS | Instant |
| 50MB multi-line log | ~8MB RSS | <2 seconds |
| 10MB source file | Full load | Immediate |

---

## Project Stats

| Metric | Value |
|--------|-------|
| C++ source lines | ~24,000 |
| Syntax languages | 96 |
| Themes | 9 built-in |
| Test suite | 148 feature tests + 70 stress tests |
| Build errors | 0 |

---

## License

MIT License — see [LICENSE](LICENSE) for details.

---

## About

Built by [Green Olive Tech LLC](https://greenolivetech.com).

*PrimeEdit — A fast native text editor for Linux.*

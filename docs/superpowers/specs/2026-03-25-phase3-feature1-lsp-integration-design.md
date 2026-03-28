# Phase 3 Feature 1: LSP Integration Design

**Project:** Olive Notepad
**Date:** 2026-03-25
**Author:** Abu Bakr (Team Lead & Architect)
**Status:** Approved

---

## Overview

Wire the existing LSPClient/LSPManager infrastructure into the editor UI. The JSON-RPC transport, process management, and protocol methods already exist (~560 lines). What's missing is: inline diagnostics rendering, hover tooltip display, go-to-definition navigation, LSP-aware completion integration, references panel, rename symbol, and per-language server configuration via settings JSON.

### Current State

- `LSPClient` (438 lines): full JSON-RPC over stdio, initialize/shutdown, didOpen/didChange/didClose/didSave, completion/hover/gotoDefinition/references requests, diagnostic notification parsing
- `LSPManager` (127 lines): singleton, per-language client management, hardcoded server configs for 5 languages (Python/pylsp, C++/clangd, JS/typescript-language-server, Rust/rust-analyzer, Go/gopls)
- `CompletionPopup`: already has `setCompletions(QVector<CompletionItem>)` for LSP items
- **Not wired:** No editor connects to LSP signals. No diagnostics rendered. No hover tooltips. No go-to-definition handling.

### Definition of Done (from brief)

1. clangd connects, hover and go-to-definition work on a C++ file within Olive Notepad itself
2. Build errors appear as red squiggles within 500ms of file save
3. LSP completions replace word-list completions when a server is active

---

## Architecture

The LSP integration is a **wiring task**, not a new subsystem. The transport layer exists. The UI components (completion popup, search results panel) exist. The work is:

1. **Editor ↔ LSP bridge** — When an editor opens/changes/saves a file, notify the LSP client. When the LSP client emits results, render them in the editor.
2. **Inline diagnostics** — Render squiggles under error/warning ranges using `QTextEdit::ExtraSelection` with wavy underline format.
3. **Hover tooltip** — Show a `QToolTip` or custom tooltip widget on Ctrl+hover or mouse hover delay.
4. **Go-to-definition** — F12 or Ctrl+click navigates to definition location (opens file if needed).
5. **LSP completion** — When LSP server is active for current language, use LSP completions instead of keyword/word completions.
6. **Find references** — Shift+F12 shows references in the search results panel.
7. **Rename symbol** — F2 prompts for new name, sends workspace/rename request.
8. **Server configuration** — JSON file at `~/.config/OliveNotepad/lsp-servers.json` for custom server paths.

---

## Components

### 1. LSP Bridge (new: `src/ui/lspbridge.h/.cpp`)

A glue class that connects MainWindow/Editor lifecycle events to LSPManager/LSPClient:

- On tab opened: detect language, if LSP server available → start server if not running → didOpen
- On tab changed: nothing (server is per-language, persists)
- On text changed: didChange (debounced 300ms to avoid flooding)
- On file saved: didSave
- On tab closed: didClose
- On cursor moved: cancel pending hover request
- Connects LSPClient signals to Editor rendering methods

### 2. Inline Diagnostics (modify: `src/ui/editor.h/.cpp`)

Add `QList<QTextEdit::ExtraSelection> m_diagnosticSelections` to Editor. New method `setDiagnostics(const QVector<Diagnostic> &diagnostics)`:
- For each diagnostic, create ExtraSelection with wavy underline (red for error, yellow for warning, blue for info)
- Add gutter icons (red circle for error, yellow triangle for warning) in the bookmark margin area
- Combine with existing extra selections via `updateExtraSelections()`

### 3. Hover Tooltip (modify: `src/ui/editor.h/.cpp`)

On mouse hover (using `QEvent::ToolTip` in event filter) or Ctrl+K:
- Get word position under cursor
- Send hover request to LSP client
- On result, show `QToolTip::showText()` with the documentation content
- Format markdown content as plain text (strip markdown syntax)

### 4. Go-to-Definition (modify: `src/ui/editor.h/.cpp`, `src/ui/mainwindow.cpp`)

F12 or Ctrl+click:
- Get cursor position
- Send gotoDefinition request
- On result: if URI matches current file, navigate to line. If different file, open it via MainWindow then navigate.

### 5. LSP Completion Integration (modify: `src/ui/editor.cpp`)

In `triggerCompletion()`:
- Check if LSP client is active for current language
- If yes: send completion request to LSP, show results in CompletionPopup using existing `setCompletions(QVector<CompletionItem>)`
- If no: fall back to keyword/word completions (existing Phase 2 behavior)

### 6. Find References (modify: `src/ui/mainwindow.cpp`)

Shift+F12:
- Send references request
- On result: populate SearchResultsPanel with reference locations
- Show the search results dock

### 7. Rename Symbol (modify: `src/ui/mainwindow.cpp`)

F2 on a symbol:
- Prompt for new name via QInputDialog
- Send textDocument/rename request (need to add to LSPClient)
- Apply workspace edit response (modify text in affected files)

### 8. Server Configuration (modify: `src/core/lspmanager.cpp`)

Load `~/.config/OliveNotepad/lsp-servers.json` on startup:
```json
{
  "c++": {"command": "clangd", "args": ["--background-index"]},
  "python": {"command": "pylsp", "args": []},
  "rust": {"command": "rust-analyzer", "args": []}
}
```
Fall back to hardcoded defaults if file doesn't exist.

---

## Files Summary

### New Files (2)
| File | Purpose |
|------|---------|
| `src/ui/lspbridge.h` | LSP ↔ Editor lifecycle bridge |
| `src/ui/lspbridge.cpp` | Bridge implementation |

### Modified Files
| File | Changes |
|------|---------|
| `src/core/lspclient.h` | Add rename request method, textDocument/rename |
| `src/core/lspclient.cpp` | Implement rename, handle rename response, handle definition array response |
| `src/core/lspmanager.cpp` | Load server config from JSON file, fallback to defaults |
| `src/ui/editor.h` | Diagnostic selections, hover event handling, go-to-definition |
| `src/ui/editor.cpp` | setDiagnostics(), hover tooltip, Ctrl+click handler, LSP completion path |
| `src/ui/mainwindow.h` | LSPBridge member, new slots for references/rename |
| `src/ui/mainwindow.cpp` | Create LSPBridge, F12/Shift+F12/F2 shortcuts, wire LSP signals |
| `CMakeLists.txt` | Add lspbridge source files |

---

## Build Sequence

1. LSP Bridge + auto-start servers on file open
2. Inline diagnostics (squiggles + gutter icons)
3. Hover tooltip
4. Go-to-definition (F12 + Ctrl+click)
5. LSP completion integration
6. Find references (Shift+F12)
7. Rename symbol (F2)
8. Server configuration from JSON file
9. Integration test: open Olive Notepad's own source, verify clangd works

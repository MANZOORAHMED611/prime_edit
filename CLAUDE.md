# PrimeEdit

## What Is This
A fast, native text editor for Linux. Built for performance, extensibility, and developer productivity.

## Status
- **Specification:** Complete
- **Implementation:** Active development — core editor, 96 syntax languages, LSP, multi-cursor, split-screen, Git integration, plugin system, remote editing all implemented

## Key Spec Documents
- `notepad-supreme-requirements.md` — Full feature requirements
- `BUILD.md` — Build instructions
- `TOOLBAR_FEATURES.md` / `TOOLBAR_LAYOUT.md` / `TOOLBAR_ICONS.md` — Toolbar specifications

## Planned Tech Stack
- **Language:** C++
- **GUI Framework:** Qt or GTK
- **Build System:** CMake (already configured)
- **Platform:** Linux (Ubuntu primary target)

## Performance Targets
- Startup time: <1 second
- Large file support: >1GB files
- Memory usage: <100MB with 10 files open

## Key Features
- Tabbed interface with multi-view / split-screen editing
- Extensive encoding support (UTF-8, UTF-16, legacy encodings)
- Syntax highlighting for major languages
- Plugin/extension system
- Integrated terminal
- Git integration
- Search and replace with regex support

## License
Open Source (GPL or MIT — to be decided)

## Project Structure
- `src/` — Source code
- `tests/` — Test files
- `resources/` — Icons and assets
- `cmake/` — CMake modules
- `html-docs/` — Generated documentation

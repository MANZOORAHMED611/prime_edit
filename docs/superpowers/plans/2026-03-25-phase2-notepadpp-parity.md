# Phase 2: Notepad++ Parity Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Complete Notepad++ feature parity: 80+ syntax languages, robust file handling with persistent unsaved documents, document map, function list, auto-completion, macro enhancements, and plugin wiring.

**Architecture:** Data-driven syntax definitions via JSON replace 13 hardcoded language methods. File handling adds charset detection, file monitoring, and session-level unsaved document persistence. UI features (document map, function list, completion) are dock widgets and popups wired into the existing MainWindow.

**Tech Stack:** C++17, Qt5/Qt6, CMake 3.16+

**Spec:** `docs/superpowers/specs/2026-03-25-phase2-notepadpp-parity-design.md`

**Build command:** `cd build && cmake .. && make -j$(nproc)`

**Run command:** `cd build && ./"Olive Notepad"`

---

## Task 1: Data-Driven Syntax Loader + First JSON Definition

**Files:**
- Modify: `src/syntax/highlighter.h`
- Modify: `src/syntax/highlighter.cpp`
- Create: `resources/syntax/cpp.json`
- Modify: `resources/resources.qrc`

### Steps

- [ ] **Step 1: Create the C++ JSON definition file**

Create `resources/syntax/cpp.json` with full C++ keyword/type/comment/string definition. Extract keyword lists from the existing `loadCppRules()` method.

- [ ] **Step 2: Add `loadFromDefinition()` to highlighter**

In `src/syntax/highlighter.h`, replace all 13 `loadXxxRules()` declarations with single `void loadFromDefinition(const QString &resourcePath)`.

In `src/syntax/highlighter.cpp`, implement: open JSON from Qt resources, parse keywords/types/builtins arrays into regex rules with appropriate formats, parse comment/string/number/operator/function/preprocessor patterns.

- [ ] **Step 3: Update `loadLanguageRules()` to use definitions**

Replace the entire if/else chain with: `loadFromDefinition(":/syntax/" + language.toLower() + ".json")`. Remove all 13 loadXxxRules implementations.

- [ ] **Step 4: Add to resources.qrc**

Add `<qresource prefix="/syntax">` section with cpp.json.

- [ ] **Step 5: Build, verify, commit**

---

## Task 2: Convert All 13 Existing Languages to JSON

**Files:**
- Create: 12 more JSON files in `resources/syntax/` (c, python, javascript, html, css, json, yaml, markdown, bash, rust, go, java, sql)
- Modify: `resources/resources.qrc`
- Modify: `src/syntax/languagemanager.h` and `.cpp`

### Steps

- [ ] **Step 1: Create JSON definitions for all 13 languages**

Extract keyword lists from removed loadXxxRules methods. Note: XML currently shares HTML rules and needs its own definition authored from scratch.

- [ ] **Step 2: Register all in resources.qrc**

- [ ] **Step 3: Add `definitionPath()` to LanguageManager**

- [ ] **Step 4: Build, verify, commit**

---

## Task 3: Add 70+ Language Definitions

**Files:**
- Create: ~70 JSON files in `resources/syntax/`
- Modify: `resources/resources.qrc`
- Modify: `src/syntax/languagemanager.cpp`

### Steps

- [ ] **Step 1: Create definitions in batches by category**

Systems, Web, Scripting, Data/Config, JVM, .NET, Functional, Mobile, Scientific, Legacy/Hardware, Other.

- [ ] **Step 2: Register all in resources.qrc**

- [ ] **Step 3: Add missing extension mappings to LanguageManager**

- [ ] **Step 4: Build, verify, commit**

---

## Task 4: Theme-Aware Syntax Colors

**Files:**
- Modify: `src/ui/theme.h`, `src/ui/theme.cpp`
- Modify: `src/syntax/highlighter.h`, `src/syntax/highlighter.cpp`
- Modify: `src/ui/editor.cpp`

### Steps

- [ ] **Step 1: Make ThemeManager a QObject with themeChanged signal**
- [ ] **Step 2: Emit signal from applyTheme()**
- [ ] **Step 3: Update highlighter setupFormats() to use theme colors, make it public**
- [ ] **Step 4: Connect Editor to themeChanged for live rehighlight**
- [ ] **Step 5: Build, verify, commit**

---

## Task 5: Charset Detector + Encoding Hardening

**Files:**
- Create: `src/core/charsetdetector.h`, `src/core/charsetdetector.cpp`
- Modify: `src/core/encoding.cpp`, `src/core/document.cpp`, `CMakeLists.txt`

### Steps

- [ ] **Step 1: Create CharsetDetector with BOM, UTF-8 validity, and byte-frequency heuristics**
- [ ] **Step 2: Harden Encoding class with QStringEncoder named encoding + fallback**
- [ ] **Step 3: Use detector in Document::load()**
- [ ] **Step 4: Add optional ICU dependency to CMakeLists.txt**
- [ ] **Step 5: Build, verify, commit**

---

## Task 6: File Change Monitoring + Notification Bar

**Files:**
- Create: `src/ui/notificationbar.h`, `src/ui/notificationbar.cpp`
- Modify: `src/core/documentmanager.h`, `src/core/documentmanager.cpp`
- Modify: `src/ui/mainwindow.cpp`, `CMakeLists.txt`

### Steps

- [ ] **Step 1: Create NotificationBar (yellow info bar with Yes/No buttons)**
- [ ] **Step 2: Add QFileSystemWatcher to DocumentManager**
- [ ] **Step 3: Wire into MainWindow (show bar on external change, reload on accept)**
- [ ] **Step 4: Build, verify, commit**

---

## Task 7: Persistent Unsaved Documents

**Files:**
- Modify: `src/core/session.h`, `src/core/session.cpp`
- Modify: `src/ui/mainwindow.cpp`

### Steps

- [ ] **Step 1: Add saveUnsavedDocuments/restoreUnsavedDocuments to Session**

Serialize each unsaved tab to JSON in `~/.local/share/OliveNotepad/sessions/unsaved/`.

- [ ] **Step 2: Modify closeEvent() to persist silently instead of prompting**

Call `Session::saveUnsavedDocuments()` before close, suppress save dialog on app quit.

- [ ] **Step 3: Modify Session::restore() to also restore unsaved docs**

- [ ] **Step 4: Build, verify, commit**

---

## Task 8: Recovery Dialog

**Files:**
- Create: `src/ui/recoverydialog.h`, `src/ui/recoverydialog.cpp`
- Modify: `src/main.cpp`, `CMakeLists.txt`

### Steps

- [ ] **Step 1: Create RecoveryDialog with checkable file list**
- [ ] **Step 2: Integrate into main.cpp before MainWindow creation**
- [ ] **Step 3: Build, verify, commit**

---

## Task 9: Reload from Disk

**Files:**
- Modify: `src/ui/mainwindow.h`, `src/ui/mainwindow.cpp`

### Steps

- [ ] **Step 1: Add reloadFromDisk slot, menu action Ctrl+Shift+R, prompt if modified**
- [ ] **Step 2: Build, verify, commit**

---

## Task 10: Document Map (Minimap)

**Files:**
- Create: `src/ui/documentmap.h`, `src/ui/documentmap.cpp`
- Modify: `src/ui/mainwindow.h`, `src/ui/mainwindow.cpp`, `CMakeLists.txt`

### Steps

- [ ] **Step 1: Create DocumentMapWidget (custom QWidget with cached QPixmap rendering)**

Each line renders as 1-2px strip. Viewport highlight rectangle. Click-to-navigate.

- [ ] **Step 2: Add to MainWindow as right dock, toggle via View > Document Map**
- [ ] **Step 3: Connect to editor text changes (debounced 500ms)**
- [ ] **Step 4: Build, verify, commit**

---

## Task 11: Function List

**Files:**
- Create: `src/ui/functionlist.h`, `src/ui/functionlist.cpp`
- Modify: `src/ui/mainwindow.h`, `src/ui/mainwindow.cpp`, `CMakeLists.txt`

### Steps

- [ ] **Step 1: Create FunctionListPanel with QTreeView, regex-based function extraction from language JSON**
- [ ] **Step 2: Add to MainWindow as left dock, toggle via View > Function List**
- [ ] **Step 3: Refresh on tab switch and text change (debounced 1s)**
- [ ] **Step 4: Build, verify, commit**

---

## Task 12: Auto-Completion

**Files:**
- Modify: `src/ui/completionpopup.h`, `src/ui/completionpopup.cpp`
- Modify: `src/ui/editor.h`, `src/ui/editor.cpp`

### Steps

- [ ] **Step 1: Add SimpleCompletionItem struct and setSimpleCompletions() to CompletionPopup**
- [ ] **Step 2: Implement gatherCompletions() in Editor (keywords from JSON + document words)**
- [ ] **Step 3: Add trigger logic (3+ chars auto, Ctrl+Space manual)**
- [ ] **Step 4: Build, verify, commit**

---

## Task 13: Macro Run N Times

**Files:**
- Create: `src/ui/macrodialog.h`, `src/ui/macrodialog.cpp`
- Modify: `src/ui/mainwindow.h`, `src/ui/mainwindow.cpp`, `CMakeLists.txt`

### Steps

- [ ] **Step 1: Create MacroDialog (run N times or until end of file)**
- [ ] **Step 2: Add menu item and runMacroMultipleTimes() slot**
- [ ] **Step 3: Build, verify, commit**

---

## Task 14: Plugin Menu + Startup Loading

**Files:**
- Modify: `src/core/pluginmanager.cpp`, `src/main.cpp`
- Modify: `src/ui/mainwindow.h`, `src/ui/mainwindow.cpp`

### Steps

- [ ] **Step 1: Add default plugin directories to PluginManager constructor**
- [ ] **Step 2: Call loadPlugins() in main.cpp after mainWindow.show()**
- [ ] **Step 3: Add Plugins menu with loaded plugin list and event wiring**
- [ ] **Step 4: Build, verify, commit**

---

## Task 15: Print Hardening

**Files:**
- Modify: `src/ui/mainwindow.cpp`

### Steps

- [ ] **Step 1: Replace printFile() with QPrintPreviewDialog implementation**
- [ ] **Step 2: Build, verify, commit**

---

## Task 16: Auto-Detection Improvements

**Files:**
- Modify: `src/syntax/languagemanager.cpp`

### Steps

- [ ] **Step 1: Add content heuristics (LaTeX, CMake, Dockerfile, XML)**
- [ ] **Step 2: Build, verify, commit**

---

## Task 17: Integration Verification

- [ ] **Step 1: Clean build from scratch**
- [ ] **Step 2: Visual verification of all features**
- [ ] **Step 3: Fix any issues, final commit**

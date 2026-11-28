# Spec: editor core migration — QPlainTextEdit → Scintilla + Lexilla (S-programme)

**Status:** revised after the Umar spec gate, round 1 (verdict `revise`, `docs/superpowers/reviews/2026-11-28-scintilla-migration-spec-verdict.md`, 470444c). The resolutions are indexed in §11. Nothing dispatches before `approve`.
**Tier:** T3. Replaces the buffer behind every save (the Phase-1-hardened save path), the `Document` ↔ widget sync, the load path, and the draft-cache text source; adds two vendored libraries and one package dependency.
**Date:** 2026-11-28 · **Author:** Abu Bakr (architecture and breakdown).
**Parent specs:** `2026-11-26-phase1-data-loss-fixes.md` (invariants I1–I5, D2, D3, D5, D7, D11, D12), `2026-11-27-pr5-review-data-safety.md` (P1.11), `2026-11-28-atomic-short-write.md` (P1.12).
**Inputs:** spike report `spikes/scintilla/REPORT.md` on branch `spike/scintilla-qt6` (`1052ccb`), the spike code beside it, and `docs/notepadpp-gap-analysis.md` §1–§2 (decision: option A; Notepad++ parity is the priority; bidi is not a deciding factor).
**fixes_artifact:** prime-edit 1.0.2 (`4d35896`): the end of a file over 50 MB is unreachable (spike §5.2). §2.3 corrects the spike's diagnosis.
**Base:** master after PR #6 (`feature/npp-qw1`) merges, **tree = `ce5d206`**. Master has not moved since `4d35896`, and the repository merges with merge commits, so the merged tree equals `ce5d206`. PR #6 adds `src/core/texttransforms.*`, the replacement adapter `Editor::textInRange` / `Editor::replaceRange` and the other members in `src/ui/editor_transforms.cpp`, the `mainwindow_npp_*.cpp` files, and a 24th suite, `test_npp_qw1`. This spec takes that adapter as the only text-replacement point to port. **Phase A dispatch stays held until PR #6 merges.** Merging it is the owner's call.

File:line references are at the `ce5d206` tree. Spike and Scintilla references are to their own sources. **[UNVERIFIED]** marks a claim I could not check by reading or running code. Each one names the workstream that verifies it first.

---

## 1. Objective

Replace the `QPlainTextEdit` editor core with Scintilla 5.6.9's own Qt layer (`ScintillaEditBase`/`ScintillaEdit`, not QScintilla) and Lexilla 5.5.5. Every editor feature in today's build keeps working, with any regression listed in §3.7.2. Every Phase-1 invariant still holds (§3.3).

The numbers this moves, stated first (spike §5, `evidence/external_matrix.txt`):

| Measure | 1.0.2 today | After (target) |
|---|---|---|
| 500 MB multi-line file (476.8 MiB) | 2 MB read-only preview; `Lines : 24804`; end unreachable; 581 ms settled; peak 161.8 MiB (165,692 KiB) | **G1:** whole file, read-only view, end reachable; settled ≤ 3.0 s; peak RSS ≤ B + 1,200 MiB; settled RSS ≤ B + 700 MiB (§3.5.2) |
| `tests/100mb.json` (one 97 MB line, 92.6 MiB) | 2 MB preview; 5,044 ms; peak 611.1 MiB | **G2:** whole file, read-only, long lines wrapped; settled ≤ 2.5 s; end reachable in ≤ 200 ms; peak RSS ≤ B + 260 MiB |
| Save byte source (UTF-8, LF file) | 3 copies (`toRawText` → PieceTable → normalise → encode, spike §6b) | 0 copies (`QByteArray::fromRawData` over the buffer) |
| Highlighting | 96 regex definitions, off above 5 MB (`editor.cpp:106`) | 77 languages on stateful Lexilla lexers, 19 on the JSON rules (§3.6); on up to 20 MiB |
| LSP `didChange` | full text on every change (`lspbridge.cpp:170`) | incremental ranges when the server advertises it |
| Startup time, idle RSS with 10 small files | not measured | **G4:** measured on the 1.0.2 and the new Release build in one run; startup < 1 s and ≤ 10 % over 1.0.2; idle RSS with 10 files < 100 MiB above B and ≤ 10 % over 1.0.2 |
| Save fast path (40 MiB UTF-8/LF save) | not measured | **G3:** VmRSS delta < 1 MiB (spike §6b: +64 kB at 256 MiB) |

**B** is the same-build baseline: the VmRSS of the new Release build with one empty untitled tab, 5 s after start, measured in the same SB6 run. 1.0.2 at the same point is measured too, for G4. Units are MiB and GiB (KiB ÷ 1024). The spike's `kB` figures are KiB.

**Gating rows.** G1–G4, and the budget-factor rows of §3.5.2 (§5.6), gate the cut-over merge. A miss of more than 15 % needs a spec amendment through the gate before the merge, never a silent change.

**Project targets** (`CLAUDE.md`, Performance Targets): startup < 1 s; files > 1 GB; < 100 MB memory with 10 files open. After S, the first and third become G4. Files over 1 GB open as whole read-only views when they fit the memory budget, and as a Preview otherwise. Editing them is F1.

Not an objective: editing files over 50 MiB. Today they are read-only previews. After the migration they are read-only **whole-file** views. Editing them is follow-up F1 (§8).

## 2. Scope

### 2.1 In

- Vendored Scintilla 5.6.9 and Lexilla 5.5.5, built from source by CMake (§3.9).
- `TextBuffer` (new, core): one Scintilla document per `Document`, created without a view. It is the authoritative text.
- `Document` re-plumbed onto `TextBuffer`: load (encoding detection, tiers, long-line fallback, memory budget), save byte source, modified flags (§3.2–§3.5).
- `Editor` reimplemented as a subclass of `ScintillaEdit` with the same public surface (the facade, §4.2), so MainWindow, Session, the plugin API and the tests do not see the swap.
- Highlighting: the language → Lexilla map, a rule-driven lexer for languages Lexilla lacks, and theme → style mapping (§3.6).
- Parity for every current editor feature (§3.7), and the integrations: LSP, git gutter, minimap, Markdown mode, split views, session, plugin API (§3.8).
- The §5.2 master bug, fixed and covered by a test.
- Packaging: `libqt6core5compat6` (Depends). `qt6-wayland` is **not** added (R1-13): native Wayland is follow-up F21.
- The test harness and ports of the existing suites, failing tests first, ASan/UBSan (§5).

### 2.2 Out

- The editor-bound **[E]** features that become cheap after the migration. They ship as follow-ups (§8).
- Editing files over 50 MiB (F1), and Clone to Other View (F2, decided in §3.8.6).
- Bidi rendering. `SCI_SETBIDIRECTIONAL` is a no-op on Qt (spike §8). The owner accepted this regression.
- Embedded CSS inside HTML, Vue and Svelte. Lexilla's `hypertext` lexer has no CSS sub-lexer (spike §7). Accepted gap; Notepad++ uses the same lexer.
- Removing `PieceTable` and `LargeFileReader`. `Document` stops using them, but `tests/validation/crash_test.cpp:235-286,383-411` and `test_all_features.cpp:62-90` test them directly. Deleting those tests is not allowed. Removal is a separate chore.
- The terminal, commit dialog, eval panel and Markdown preview keep their own `QPlainTextEdit`/`QTextBrowser`. They are not editors of user files.
- A Qt 5 build. `CMakeLists.txt:21-36` still has a Qt 5 fallback, but no CI job builds it. The Scintilla glue links `Core5Compat` only on Qt 6, and Qt 5 is not a gate.

### 2.3 Findings that correct earlier documents

The highest-priority report. Each item was verified by grep or by reading the code.

1. **The §5.2 root cause is not the scrollbar range.** `Document::m_largeFileReader` is initialised to `nullptr` (`document.h:155`), and nothing in `src/` assigns it (grep `m_largeFileReader\s*=` finds only the declaration). `isLargeFile()` is therefore always false, and `Editor::loadViewportContent` (`editor.cpp:2892-2941`) and the scrollbar wiring (`editor.cpp:112-132`) are dead. A LargeFile document only ever holds the 2 MB preview loaded at `editor.cpp:198-227`. The end is unreachable because it was never loaded. The fix (§3.5) loads the whole file. No viewport reader is revived.
2. **Rainbow brackets are unreachable.** `m_rainbowBrackets` defaults to false (`editor.h:283`), and `setRainbowBrackets` has no caller outside `editor.cpp`. The gap analysis (§0, §5) lists them as present.
3. **LSP hover is unreachable.** `Editor::requestHover` (`editor.cpp:2852`) has no caller anywhere in `src/`. The gap analysis lists hover as present.
4. **The LSP client ignores the server's `textDocumentSync` capability.** `lspclient.cpp` never reads it (grep). It always sends full text.
5. **Test counts.** 23 suites on `4d35896`: 20 regression suites, `test_runner`, `crash_test` and `tst_markdown`. With npp-qw1 there are 24. The files using `QTextCursor`/`QTextBlock` number 12 on `4d35896` (listed in §5.3) and 13 with `test_npp_qw1.cpp`. The spike said 13.

## 3. Design

### 3.1 Components after the change

| Component | Files | Responsibility after S | Owner |
|---|---|---|---|
| Vendored Scintilla, Lexilla | `third_party/scintilla/`, `third_party/lexilla/`, `third_party/SCINTILLA_PIN`, `third_party/MANIFEST.sha256` | Pinned upstream sources (§3.9), plus the generated `ScintillaEdit.{h,cpp}`. Never edited. | abdullah |
| CMake glue | `cmake/Scintilla.cmake` (new), `CMakeLists.txt` | Static targets `scintilla_qt` and `lexilla`, linked into `prime-edit-core`, with warnings off for third-party sources. | abdullah |
| TextBuffer (new) | `src/core/textbuffer.{h,cpp}` | Owns one ref-counted `Scintilla::Internal::Document` (code page UTF-8). Load through its `ILoader` face; 64-bit length, buffer and range pointers; save point; read-only; undo; change notifications as Qt signals through a `DocWatcher`. The **only** file that includes Scintilla internal headers. | uthman |
| Document | `src/core/document.{h,cpp}` | Unchanged role: path, encoding, line ending, format-dirty, read-only, mode, save. Now owns a `TextBuffer` instead of a `PieceTable`. Load builds a new buffer (§3.5). Save takes its bytes from the buffer (§3.4). | uthman |
| Utf8Scan (new) | `src/core/utf8scan.{h,cpp}` | Byte-level, allocation-free passes over a loaded `QByteArray`: strict UTF-8 validity, in-place EOL normalisation with line-ending counts, NUL-beside-break check, longest line, and the reflow of long lines. | uthman |
| LspChangeTracker (new) | `src/core/lspchangetracker.{h,cpp}` | Per open document. Turns buffer change notifications into queued LSP content changes (line, UTF-16 column). | uthman |
| LSPClient | `src/core/lspclient.{h,cpp}` | Reads `textDocumentSync` from the initialize result; sends incremental or full `didChange`. | uthman |
| Session | `src/core/session.cpp` | Calls only the Editor facade (Phase A). No format change. | uthman |
| Editor | `src/ui/editor.h`, `src/ui/editor*.cpp` (split, each ≤ 500 lines) | A subclass of `ScintillaEdit` that attaches to its `Document`'s `TextBuffer`. Keeps the facade (§4.2). Holds the feature behaviour in §3.7. | ali |
| KeyChordDispatcher (**conditional**, R1-12) | `src/ui/keychords.{h,cpp}` | Built only if `test_s0_probe` case (b) fails (§5.1). It would map a synthetic `(key, modifiers)` to the SCI command Scintilla's default key map assigns, with the table generated from vendored `KeyMap.cxx` `MapDefault[]`. | ali, only if needed |
| Editor ids (new) | `src/ui/editorids.h` | Margin, marker, indicator and style numbers (§4.4). | ali |
| LexerCatalog (new) | `src/syntax/lexercatalog.{h,cpp}` | Language id → Lexilla lexer name, keyword sets and properties, or the JSON rules (§3.6); `commentSyntax(language)` (§3.7.1 row 44). Created in SB2 (lexer, `fold=1`, `commentSyntax`); extended in SB3 (keywords, JSON rules) (R1-5). | ali |
| JsonRuleLexer (new) | `src/syntax/jsonrulelexer.{h,cpp}` | An `ILexer5` built on Lexilla's `DefaultLexer`. It drives the existing `resources/syntax/*.json` rules and today's fold heuristic. | ali |
| ThemeStyleMap (new) | `src/syntax/themestylemap.{h,cpp}` | Theme → Scintilla styles, elements, markers and indicators, per view. | ali |
| SyntaxHighlighter | `src/syntax/highlighter.{h,cpp}` | Deleted. Its rule loading moves into JsonRuleLexer. | ali |
| LineNumberArea | `src/ui/linenumberarea.{h,cpp}` | Deleted. Margins replace it. | ali |
| DocumentMapWidget | `src/ui/documentmap.{h,cpp}` | A second `ScintillaEdit` view on the same Scintilla document (§3.8.3). | ali |
| MarkdownModeController, LSPBridge, MainWindow, StatusBar, FunctionList | `src/ui/...` | Facade callers (Phase A). In Phase B: the minimap wiring, view-kind notices, and the LSP flush. | ali; LSPBridge uthman |

### 3.2 Buffer ownership

**Decision: the Scintilla document is the only copy of the text. It is owned by the `Document`, not by a widget.**

- `Document` holds `std::unique_ptr<TextBuffer> m_buffer`. Every `Editor` showing the document, and the minimap, attaches to it with `SCI_SETDOCPOINTER(0, buffer->docPointer())` (`Editor.cxx:8444-8447`). The document's reference count keeps it alive for every view. `Document` holds one reference.
- Why the `Document` and not the widget: Regression suites call `doc.setText(...)` and then `doc.save()` with no editor (`test_t1_1_save.cpp:56`, `test_t1_12_short_write.cpp:93`, `test_t1_11a_direct_write.cpp:52`). Nine test files call `Document::setText` directly — seven regression suites, `crash_test` and `test_all_features` — and so does `tests/regression/tools/sync_failure_driver.cpp`. The draft writer also reads documents. A widget-owned buffer would force a hidden widget into core.
- **Construction without a view** follows `ScintillaDocument.cpp:99-108` and the `CreateLoader` handler at `Editor.cxx:8468-8476`: `new Document(options)`, `AddRef`, `Allocate`, `SetUndoCollection(false)`, `AddData` in 16 MiB chunks, `ConvertToDocument`, `SetUndoCollection(true)`, `SetDBCSCodePage(SC_CP_UTF8)`. `TextBuffer` implements `DocWatcher` itself instead of using `ScintillaDocument`, whose API uses `int` positions (`ScintillaDocument.h`). **[UNVERIFIED: the spike reached the buffer only through view messages. Building and loading a document with no view is read from `Document.h:336,464-466,581` but was never compiled. SB1 proves it first, in `test_s1_buffer`.]**
- **Read-only lives on the document.** `SCI_SETREADONLY` sets `pdoc->SetReadOnly` (`Document.h:526`), so it is shared by every view. `Document::setReadOnly` sets `TextBuffer::setReadOnly`, and views never call `SCI_SETREADONLY`. The minimap therefore must not set it (§3.8.3).
- **Programmatic edits ignore read-only, as today.** `Document::setText/insert/remove` lift the buffer's read-only flag for the duration of the edit. Today they write the PieceTable whatever the flag says (`document.cpp:566-585`), and the bypass keeps that semantics. (`test_t1_1_save.cpp:196-210` calls `setText` *before* `setReadOnly`, so it does not depend on the bypass.) Read-only blocks **user** edits only (D12).
- **`PieceTable`.** `Document::content()` and the `PieceTable` member are removed. `text()` is now `QString::fromUtf8` over the buffer. `setText`, `insert(qint64, QString)`, `remove(qint64, qint64)`, `undo`, `redo`, `canUndo` and `canRedo` keep their signatures. Positions stay UTF-16 code units and are converted inside `Document`. The `PieceTable` class stays in the tree (§2.2).
  - **Undo semantics change (R1-17b).** `setText` becomes one undoable buffer action (`replaceAll`). Today `Document::undo` always sets modified (`document.cpp:587-592`). After S, `setText` followed by `undo` on a clean document returns to the save point and reads clean, because the text really is back to the saved state (I2). No existing suite calls `Document::undo` after `setText` (grep); S1 case 7 asserts the new behaviour.
- **Medium and Large modes** become the tiers in §3.5.2. `FileMode {SmallFile, MediumFile, LargeFile}` and `setSizeThresholdsForTest(medium, large)` keep their meaning: Small is full features, Medium is editable but reduced, Large is a read-only view. The default for `MEDIUM_FILE_THRESHOLD` stays 20 MiB and for `LARGE_FILE_THRESHOLD` 50 MiB.
- **`syncToDocument` / `syncFromDocument` / `runSyncedEdit`.** Signatures are kept, because MainWindow, Session and 3 suites (`test_t1_2_flags`, `test_t2_2_sync`, `test_t3_4_reload`) call them.
  - **Transitional bridge.** Phase A adds `Document::replaceContent(const QString &)`. It copies text in without touching any modified flag (today's `PieceTable::setText`). In SB1 that means `replaceAll`, then `setSavePoint()` again if the buffer was at its save point before the call (R1-17a), so the copy itself never changes `isModified()`. `syncToDocument` calls it instead of `content()->setText` (`editor.cpp:1333`). Between the SB1 and SB2 merges, the old `QPlainTextEdit` editor keeps working on top of a `TextBuffer`-backed `Document`: save syncs into the buffer, load reads `text()`, and the `modificationChanged` hook maps to `Document::setModified`. The integration branch therefore stays green after SB1 alone.
  - `syncToDocument()` becomes a no-op in SB2: there is nothing to copy. It stays a no-op for this programme, and removing it is a chore.
  - `syncFromDocument()` re-attaches the editor to `document()->buffer()` if the buffer was replaced (after load, reload or Reopen with Encoding). Otherwise it does nothing.
  - `runSyncedEdit(target, edit)` runs `edit` and then applies `target` to the `Document` flags below. There is no second flag to keep in step any more.
- **Modified flag (I2).** `Document::isModified() = contentModified || formatDirty || draftDirty`.
  - `contentModified = !buffer.isSavePoint()`. Scintilla maintains it across typing, undo and redo, and it "detaches" when an edit is made after undoing past the save point, so undo cannot fake clean.
  - `formatDirty` is unchanged (`document.cpp:627-633`).
  - `draftDirty` (new) is set by `SyncTarget::Modified` (draft restore). It replaces `QTextDocument::setModified(true)`'s clean-index reset (`session.cpp:453`, `editor.cpp:1340-1367`). Only a successful save or load clears it.
  - `TextBuffer` emits `savePointChanged(bool)` from `DocWatcher::NotifySavePoint` (that is, `SCN_SAVEPOINTREACHED`/`LEFT` at document level). `Document` emits `modifiedChanged(isModified())` whenever the OR changes.
  - `SyncTarget::Clean` calls `buffer.setSavePoint()` and clears `draftDirty`; `formatDirty` is untouched, as today. `Modified` sets `draftDirty`. `Preserve` records the three components before `edit` and restores them: `setSavePoint` if the content was clean, and `draftDirty` if it was modified but the edit reached the save point. **`Preserve` loses its only production caller** — the RTL pass, `editor.cpp:307`, deleted in SB2 (R1-17c). It is kept for the contract and for `test_t1_2_flags.cpp:186`.
  - `Document::setModified(bool)` (a public slot that tests call): `false` behaves as `Clean`, `true` as `Modified`.
  - The Editor has **no** modified flag of its own. The `QTextDocument::modificationChanged` hook (`editor.cpp:137-142`) disappears. Save, the close prompt and the draft cache now read one source by construction, which strengthens I2.
- **Undo history** lives in the document. Moving a tab to the other pane (a new `Editor` on the same `Document`) keeps undo. Today it is lost with the widget.

### 3.3 Phase-1 invariants restated

| Invariant | Mechanism today | Mechanism after S | Proof |
|---|---|---|---|
| **I1** round trip through `MainWindow::saveFile` | `rawText` → PieceTable → `normalizeLineEndings` → `TextCodec::encode` (`document.cpp:244-251`) | Load normalises EOLs to LF exactly as `adoptLineEndings` does (`document.cpp:478-489`), byte-wise for UTF-8 (proven equal by S1 case 4). Save takes the zero-copy view when the encoding is UTF-8, the line ending is LF and the buffer holds no `\r`. Otherwise it does one encode pass that is byte-identical to today's (§3.4). | `test_i1_roundtrip` unchanged, plus S1 cases 1–5 |
| **I2** modified tracking | two flags synced by `runSyncedEdit` | one OR (§3.2): Scintilla save point, `formatDirty`, `draftDirty` | `test_t1_2_flags`, `test_t2_1_modified` (amendment A1 only), `test_t2_2_sync`, plus S1 case 7 |
| **I3** no lossy write without consent | `refusalFor` and `EncodeLossy` (`document.cpp:221-250`) | Unchanged. The fast path runs only for UTF-8 targets, and invariant B1 makes the buffer valid UTF-8, so nothing is unmappable. Legacy targets still go through `TextCodec::encode`. | `test_t1_3_encoding`, `test_t3_3_encoding_ui` |
| **I4** durable writes | `writeAtomic` (unbuffered `QSaveFile` + checked `fdatasync`) and `writeDirect` | **Unchanged code.** Only the `QByteArray` argument changes source. | `test_t1_1_save`, `test_t1_11a_direct_write`, `test_t1_12_short_write`, unchanged |
| **I5** unsaved work survives | draft writer over `allEditors()`; `QTextDocument::isEmpty` content check (`session.cpp:278-288`) | Same writer. `needsCaching` uses `Editor::isEmpty()` (the buffer length), and `draftEntry` uses `rawText()` and the facade (UTF-16 `cursorPosition`, first visible line). Every editable document is ≤ 50 MiB, as today, so draft cost does not grow (§1, F1). | `test_t1_5_cache`, `test_t1_10_session_autosave`, `test_t1_11b_drafts`, `crash_test` |
| **D1** programmatic edits never fake a flag; per-site targets | `runSyncedEdit` targets: load/reload `Clean`, draft restore `Modified`, RTL `Preserve`; the Editor constructor applies `Modified` or `Clean` on bind (`editor.cpp:97-98`) | Load and reload build a new buffer whose save point is set by the loader (`Clean`); draft restore sets `draftDirty` (`Modified`); `Preserve` has no production caller (§3.2). **The Phase B Editor constructor applies no `SyncTarget`: it only attaches** to the buffer. That matters for Move to Other View: a `Modified` on bind would set `draftDirty`, and undo could then never reach clean again. Phase A keeps today's constructor, because `QPlainTextEdit` still needs it. | `test_t1_2_flags`, `test_t2_1_modified`; S4 case "Move keeps undo to clean": type, move to the other pane, undo, and the document reads clean |
| **D4/D6** draft restore order; cached encoding applied | `placeDraft` applies `setEncoding` and `setLineEnding`, then `runSyncedEdit(Modified, setText + syncFromDocument)` (`session.cpp:445-456`) | Same order and code. `setText` replaces the text inside the *existing* buffer (no new buffer), so `syncFromDocument` has nothing to re-attach. `draftDirty` keeps the draft modified through type plus undo. | `test_t1_5_cache`, `test_t1_11b_drafts` |
| **D8** own save not reported as external; reload | `Document::saved(path, sha1, size)` hashes `written`; reload via `runSyncedEdit(Clean, syncFromDocument)` (`mainwindow_extended.cpp:205, 386`) | `saveAs` computes `sha1` and `size` from the bytes it wrote, while the view is still valid, and passes them to `finishSave` (B2). Reload builds a new buffer (`bufferReplaced`); `syncFromDocument` re-attaches inside the same `Clean` target. | `test_t1_7_watch`, `test_t3_4_reload` |
| **D9** a file with no line break takes the default ending | `detectLineEnding` returns `defaultLineEnding()` when it counts no breaks (`document.cpp:706`) | `Utf8Scan::normaliseEols` returns the three counts, and `Document` applies the same rule: zero breaks gives `defaultLineEnding()`. | S1 case 4 adds an empty file and a no-break file; `test_i1_roundtrip` empty and no-newline rows |
| **P1.11 F2–F4** failed draft write at quit asks; no write during restore; draft never lands in a read-only document | `saveUnsavedDocuments` returns `bool`; `m_restoringDrafts` guard; `placeDraft` routes `isReadOnly() \|\| isPartialView()` to an untitled tab (`session.cpp:429-431`) | Unchanged code. F4 now also routes drafts whose original opens as a LargeView or LongLineView, because both are partial views. | `test_t1_11b_drafts`; S1 case 17: a draft whose original has a line over `L` opens as a LongLineView, and the draft lands in a writable untitled tab |
| **D2** folding is view-only | `QTextBlock::setVisible` | Scintilla folding hides lines through the per-view contraction state and never edits text. `SC_AUTOMATICFOLD_SHOW` plus `SCI_ENSUREVISIBLEENFORCEPOLICY` reveal a fold before find, go-to and replace-all touch it. | `test_t2_3_folding` (ported; amendment A3 only if the fold extent differs) |
| **D3** partial views are not saveable | `isPartialView() = LargeFile` | Unchanged definition. LargeFile now covers the three read-only view kinds in §3.5.2. `refusalFor`, menu disabling and the conversion refusal are unchanged. | `test_t1_1_save` 51 MB and 21 MB cases, `test_t2_4_readonly`, `test_t3_2_save_ui` (amendment A2: wording only) |
| **D5** one editor per document; lifetime guard | Clone removed; scan in `closeFile` | Unchanged. The minimap is a `ScintillaEdit`, not an `Editor`, so `allEditors()` never lists it. Clone stays out (§3.8.6). | `test_t3_1_split` |
| **D7** flags cleared only on `Ok` | `finishEditorSave` → `runSyncedEdit(Clean)` (`mainwindow.cpp:916-919`) | Unchanged. `Document::finishSave` also calls `setSavePoint`; the second `Clean` is harmless. | `test_t3_2_save_ui`, `test_t2_2_sync` |
| **D11** text fidelity (U+00A0, U+2028) | `toRawText` with U+2029 → LF | The buffer stores UTF-8 bytes verbatim. `SCI_SETLINEENDTYPESALLOWED` stays `SC_LINE_END_TYPE_DEFAULT`, so U+2028/U+2029 are characters, not line ends. Enabling Unicode line ends is forbidden: it would shift line numbers against LSP and the file. | `test_t3_4_reload`, `test_t3_3_encoding_ui` NBSP cases |
| **D12** read-only is enforced | `Editor::event` routes keys to the base class when read-only | The buffer's read-only flag blocks every user edit path: typing, paste, drop, IME, Scintilla commands. Navigation and selection still work natively. | `test_t2_4_readonly` |
| **P1.11 / P1.12** direct write, short write, fdatasync | `writeDirect`, `writeAtomic` | Unchanged code | the two suites, unchanged |

**New invariants:**

- **B1. The buffer is always valid UTF-8.** Bytes enter it only through: the loader, after strict validation or a `QString::toUtf8` of decoded text (§3.5). A failed strict validation never loads raw bytes: it falls through to the decode path, as a lossy decode opened read-only. That covers both bytes the lenient detector called UTF-8 and a forced Reopen with Encoding as UTF-8 (R1-2). A Preview's 2 MiB cut never splits a code point (§3.5.2); `Document`/`Editor` APIs that take a `QString`; and Scintilla's own Qt input paths (typing, IME, paste, drop), which convert from `QString`. Nothing ever calls `SCI_ADDTEXT`/`SCI_APPENDTEXT` with raw file bytes. A debug-build assertion validates the buffer in `Document::saveAs`.
- **B2. The zero-copy view lives only for one synchronous `saveAs`, and no signal fires while it is live (R1-1).** The pointer is taken inside `saveAs`. `writeAtomic`/`writeDirect` emit nothing and open no dialogs. After a successful write, `saveAs` computes `sha1` and `size` from the view and then **drops the view** (the `QByteArray` and the pointer). Only then does it call `finishSave(path, sha1, size)` (a new private signature), which emits `filePathChanged`/`modifiedChanged`/`saved` and calls `setSavePoint`. A slot that edits the document during those signals therefore cannot invalidate a live pointer. `MainWindow::confirmDirectWrite` runs **between** two `saveAs` calls (`mainwindow.cpp:897-903`), and each call takes a fresh pointer. The guard is a debug assertion, not a read-only flag: `TextBuffer::changeCount()` (incremented by its `DocWatcher` on every modification) must be equal when the pointer is taken and when the view is dropped. The read-only guard is withdrawn, because §3.2 lets programmatic edits lift read-only.
- **B3. The buffer is LF-only after load, except in T1.8 read-only documents** (text that looks like misdecoded UTF-16 keeps its `\r`, `document.cpp:478-486`; harmless because the document is read-only, and caught by the fast-path guard anyway) (R1-18). Typed Enter inserts LF (`SCI_SETEOLMODE(SC_EOL_LF)`), and pastes are converted (`SCI_SETPASTECONVERTENDINGS(true)`). Programmatic inserts may still add a `\r`, so the fast path checks with `memchr` instead of assuming (§3.4).

### 3.4 Save path

`writeAtomic(path, const QByteArray &, WriteMode)` and `writeDirect(path, const QByteArray &)` (`document.cpp:272-338`) do not change by one line. `saveAs` (`document.cpp:230-270`) changes only where `data` comes from. Lines 244–251 become `const EncodeResult encoded = encodeForSave();`:

1. `p = buffer.bufferPointer()` (internally `Document::BufferPointer`, which `SCI_GETCHARACTERPOINTER` returns, `Editor.cxx:8704-8705`). It closes the gap, and `n = buffer.length()`.
2. **Fast path:** if the encoding is `UTF-8`, the line ending is `Unix`, and `memchr(p, '\r', n) == nullptr`, then `bytes = QByteArray::fromRawData(p, n)`: no copy. For `UTF-8-BOM` under the same conditions, `bytes = BOM + view`: one copy.
3. **UTF-8, another line ending, or a stray `\r`:** one byte pass into a new array. CRLF/CR/LF become the target ending, matching `normalizeLineEndings` byte for byte (S1 case 4). The BOM is prepended if named.
4. **Any other encoding:** `TextCodec::encode(normalizeLineEndings(QString::fromUtf8(p, n), m_lineEnding), m_encoding)`. Unmappable characters return `EncodeLossy`, exactly as today.
5. On `Ok`: compute `sha1` and `size` from the bytes written, drop the view, then call `finishSave(path, sha1, size)`. It does what it does today, and also calls `buffer.setSavePoint()` and clears `draftDirty` (B2).

**Regression gate:** `test_t1_1_save`, `test_t1_11a_direct_write`, `test_t1_12_short_write` (including the `RLIMIT_FSIZE` sweeps, the limit-0 runs and the draft-cache case) and `test_i1_roundtrip` pass **without edits**, beyond the Phase A mechanical port in §5.3. The P1.12 strace receipt is re-run: one save still shows a single `write`, then `fdatasync`, then `rename`.

### 3.5 Loading

#### 3.5.1 Pipeline (`Document::load`)

1. **Pick a candidate tier by size** (§3.5.2). Check the budget with the UTF-8 peak factor; the encoding is not known yet. If that check fails, read only the first 2 MiB (a Preview). Otherwise read the file into one `QByteArray` (`readAll`), as today. Step 2 can still demote the file to a Preview: when the detected encoding is legacy and its larger factor breaks the budget, the array is dropped and only the first 2 MiB are kept.
2. **Detect the encoding on the whole array** with the unchanged `CharsetDetector::detectCharset` (`charsetdetector.cpp:11-37`). It is a linear pass with no allocation, and it already runs on whole files in Small and Medium. Its order stays: BOMs, then BOM-less UTF-16, then UTF-8, then legacy. A Large view now also detects on the whole array; only a Preview keeps the 8 KB probe (D6 amended to match).
3. **UTF-8 (with or without BOM): no `QString`.**
   The steps run in this order (R1-18):
   1. `Utf8Scan::validate` is strict: it rejects overlongs, surrogates, values above U+10FFFF and truncated sequences. Its verdict equals `TextCodec::decode(bytes, "UTF-8").lossless`, and S1 case 3 proves the two equal on a corpus. **If it fails, go to step 4** (lossy decode, read-only). Raw bytes are never loaded (B1, R1-2).
   2. The BOM is skipped.
   3. `hasNulBesideLineBreak` is evaluated on the bytes **before** any normalisation. If it trips, the T1.8 rule applies, exactly as `adoptLineEndings` does: no normalisation, default ending, read-only. The check is equivalent for valid UTF-8, because U+0000, CR and LF are single bytes.
   4. `Utf8Scan::normaliseEols` works in place (the output is never longer than the input). It counts CRLF, LF and CR exactly as `detectLineEnding` does (`document.cpp:688-710`), including the D9 rule for zero breaks.
   5. The longest line and the UTF-16 length are recorded (§4.2).
4. **Any other encoding:** `TextCodec::decode` → `adoptLineEndings` (unchanged) → `toUtf8()`. Same verdicts as today.
5. **Pick the final tier:** a longest line over `L` (default 1 MiB, §3.5.3) forces LargeFile/LongLineView. The memory budget (§3.5.2) can force a Preview.
6. **Build a new `TextBuffer`** through the loader in 16 MiB chunks:
   - Small: `SC_DOCUMENTOPTION_DEFAULT`.
   - Medium and Large: `SC_DOCUMENTOPTION_STYLES_NONE | SC_DOCUMENTOPTION_TEXT_LARGE`.
   - Free the read array.
   - Set the save point and `readOnly` per tier.
   - Emit `bufferReplaced()`. Editors re-attach in their `bufferReplaced` slot and keep the caret line when it still exists.
7. A lossy decode, or the T1.8 NUL safety net, makes the document read-only, as today (`document.cpp:443-489`).

#### 3.5.2 Tiers and memory budget

**Factors (R1-7).** The spike's figures are KiB, measured on 500,000,016 bytes (488,281 KiB), and they include the demo's own baseline. The spike streamed the file into the loader in 16 MiB chunks, with no whole-file array (`demo/bench.cpp:69-81`).

| `STYLES_NONE` | Styled |
|---|---|
| VmHWM 681,008 KiB → peak factor **1.39** | VmHWM 1,169,724 KiB → **2.40** |
| VmRSS after load 637,044 KiB → settled factor **1.30** | 1,125,408 KiB → **2.30** |

PrimeEdit keeps today's whole-file `readAll` (§3.5.1 step 1), because `CharsetDetector` and `TextCodec` need the whole array. The load peak therefore adds that array, +1.0 × size. The factors below include it. They are per byte of file and above **B** (§1).

For legacy encodings the decode holds the array (1×), the UTF-16 text (2×) and the UTF-8 re-encode (up to 2× for Arabic in windows-1256) at once, before the buffer is built:
- single-byte legacy: ≤ 5.0×;
- UTF-16: ≤ 3.5× (array 1×, text 1×, UTF-8 ≤ 1.5×).

These two are derived, not measured. The UTF-16 line-character index (§4.2) adds about 8 bytes per line, about 44 MiB at 500 MB with 80-byte lines (estimate; SB6 measures it).

| Mode / view kind | Trigger | Text in buffer | Editable | Lexer, styles, folding | `isPartialView` | Settled RSS above B | Load peak above B |
|---|---|---|---|---|---|---|---|
| SmallFile | size ≤ medium (20 MiB), longest line ≤ L | whole | yes | Lexilla or JSON rules; styles; folding | no | ≤ 2.30 × size (≤ 46 MiB at 20 MiB) | ≤ 3.40 × size (UTF-8); ≤ 5.0 × (single-byte legacy); ≤ 3.5 × (UTF-16) |
| MediumFile | medium < size ≤ large (50 MiB), longest line ≤ L | whole | yes | none (`STYLES_NONE`); no folding | no | ≤ 1.30 × size + index (≤ 70 MiB at 50 MiB) | ≤ 2.39 × size (UTF-8); legacy as above |
| LargeFile · **LargeView** | size > large, within budget, UTF-16 length ≤ 2³¹ − 1 | whole | no | none | yes | 1.30 × size + index (G1: ≤ 700 MiB at 500 MB) | 2.39 × size + index (G1: ≤ 1,200 MiB at 500 MB) |
| LargeFile · **LongLineView** | longest line > L (any size), within budget, UTF-16 length ≤ 2³¹ − 1 | whole, with long lines wrapped (§3.5.3) | no | none | yes | ≈ 1.30 × size | ≈ 2.39 × size (G2: ≤ 260 MiB for `100mb.json`) |
| LargeFile · **Preview** | projected peak > budget, UTF-16 length > 2³¹ − 1, or size > `large` beyond the budget hook (tests) | the first 2 MiB, cut on a code-point boundary, long lines wrapped | no | none | yes | ≤ 5 MiB | ≤ 5 MiB |

- **Budget.** The projected load peak must be ≤ 60 % of `MemAvailable` from `/proc/meminfo`, read at open, using the table's peak factor for the detected encoding. A file over the budget opens as a Preview with today's "only part of it is loaded" notice. Test hook: `Document::setMemoryBudgetForTest(qint64 bytes)` (−1 restores the automatic value).
- **UTF-16 length cap (R1-9).** The facade's positions are `int` UTF-16 units (§4.2), so a whole-file view is allowed only while the text's UTF-16 length fits in `int`. The length is counted during validation (or taken from the decoded `QString`). Over the cap, the file opens as a Preview.
- **Preview cut (R1-2).** The 2 MiB cut is moved back to a code-point boundary for the encoding the 8 KB probe detected:
  - UTF-8: never before a continuation byte;
  - UTF-16: an even byte count, and never between a high and a low surrogate;
  - UTF-32: a multiple of 4.

  Legacy multibyte encodings are cut at 2 MiB and decoded through `TextCodec`. Any trailing partial character becomes U+FFFD, which `toUtf8` keeps valid (B1).
- **Medium loses folding.** Today Medium (20–50 MB) has heuristic brace/indent folding. This is R2 in §3.7.2. Highlighting was already off above 5 MB.
- **Medium gains editing of long lines (R1-19c).** Today a Medium file with a line over 8,000 characters opens read-only (`document.cpp:111-121`). After S, lines up to `L` stay editable. Change C7 in §3.7.2.
- **SB6 verifies the factors.** It measures VmHWM and settled VmRSS at 1, 20, 50, 100 and 500 MiB (UTF-8), on `tests/100mb.json`, and on 100 MiB **windows-1256** and **UTF-16LE** files (R1-7). The budget check relies on these factors, so they are gating rows: a measured factor more than 15 % above the table needs a spec amendment, which corrects the factor before the cut-over merge.

#### 3.5.3 Long-line fallback

- **Why.** Scintilla lays out a whole line before it paints it. The 97 MB single line froze for 55 s, with or without layout threads (spike §5.1).
- **Threshold `L` (R1-8).** The default until measured is 1 MiB. SB6 sweeps single-line files of **64 KiB, 256 KiB, 512 KiB and 1 MiB** in the Small tier, with styles and a lexer, which is the editable worst case. For each it measures:
  - first paint after open;
  - keystroke-to-paint when typing at the end of the long line (median and maximum of 20 keystrokes).

  `L` is the largest swept value whose first paint is ≤ 500 ms **and** whose keystroke-to-paint maximum is ≤ 50 ms. The values and the chosen `L` are recorded in the receipt and in `document.h`. If even 64 KiB fails, SB6 stops and reports: a spec amendment decides. `L` never exceeds 1 MiB. Test hook: `Document::setLongLineThresholdForTest(qint64)`.
- **`L` is enforced only at load (R1-16).** A line longer than `L` can still enter an editable document through paste, Join Lines or draft restore (drafts are restored into editable tabs, §3.3 P1.11 F4). It stays editable and may be slow. SB6 measures restoring a draft with a 5 MiB line (time to first paint, keystroke-to-paint) and records it as known limit **R5** (§3.7.2). Enforcing after load is F22.
- **LongLineView.** `Utf8Scan::reflow` inserts an LF after every 4,096 bytes of any line longer than 4,096 bytes, always on a UTF-8 code-point boundary (never before a continuation byte). The buffer is therefore not the file. That is why it is a partial view (D3): save is refused, conversions are off, and Reopen with Encoding is off.
- **The notice** names it: "'%1' has lines too long to edit. It is shown read-only, with long lines wrapped every 4,096 bytes; copied text contains those breaks." `isMinified()` is true exactly in this view.
- **Test on `tests/100mb.json`**, in two halves:
  - **S1 case 9a (Document level):**
    - `viewKind() == LongLineView`;
    - `isPartialView()`;
    - the buffer's last line equals the file's last `n mod 4096` bytes;
    - `load` takes ≤ 2.5 s.
  - **S2 case 9b (Editor level):**
    - `send(SCI_DOCUMENTEND)` puts the caret on the last line;
    - wall time from `openFile` to the first `paintEvent` is ≤ 2.5 s;
    - `SCI_DOCUMENTEND` takes ≤ 200 ms.

  All times are printed, and asserted with the margin in §5.6.

#### 3.5.4 The §5.2 bug

Fixed by LargeView: the whole file is in the buffer.

Two cases are written first, and both must fail on the base. They use a generated 60 MiB file of 80-byte numbered lines.

- **S1 case 8a (Document level):** after `load`, `viewKind() == LargeView` and `text()` ends with the expected last line. On the base, a LargeFile document holds no text, so `text()` is empty.
- **S2 case 8b (Editor level):**
  1. `MainWindow::openFile`.
  2. `QTest::keyClick(editor, Qt::Key_End, Qt::ControlModifier)`.
  3. Assert that `editor->currentLine()` is the last line number and its text is the expected last line.

  On the base, `currentLine()` stops at the end of the 2 MB preview (spike: line 24,804). The test uses only API that exists on both sides (`currentLine`, `keyClick`).

### 3.6 Highlighting

- **LexerCatalog** is a static table: language id (resolved as `SyntaxHighlighter::loadLanguageRules` resolves names today, so "C++" and "cpp" both work) → `{lexilla name, keyword sets, properties}` or `json`. It also serves `commentSyntax(language)` from the JSON definition's comment fields (§3.7.1 row 44).
  - **Split across two tasks (R1-5).** SB2 creates it with the lexer name, `fold=1` and `commentSyntax`, and applies default styles. That is enough for `test_t2_3_folding` (lexer fold levels) and for `test_npp_qw1`'s `commentSyntax` case to pass in SB2. SB3 adds the keyword sets, JsonRuleLexer and themes. JSON-rule languages get no lexer between SB2 and SB3 (plain text, no folding); no suite needs them there.

  `apply(Editor*, language, FileMode)`:
  - sets nothing for Medium and Large;
  - otherwise calls `SCI_SETILEXER` with `CreateLexer(name)` (Lexilla linked statically, no dlopen) or a new `JsonRuleLexer`;
  - sets `SCI_SETKEYWORDS` from the language's JSON definition: set 0 ← `keywords`, set 1 ← `types` + `builtins` when the lexer has a second set;
  - sets properties: `fold=1`, plus `lexer.css.less.language=1` or `lexer.css.scss.language=1` for Less and SCSS.

  The lexer is per document (`SCI_SETILEXER` sets the document's lex interface), so the minimap gets the same styling.
- **Rule for choosing Lexilla.** A language goes to a Lexilla lexer when it is that lexer's own language, the lexer Notepad++ uses for it, or a C-syntax language on the `cpp` lexer (comments `//` `/* */`, strings `"` `'`). Otherwise it keeps its JSON definition. Being conservative here avoids trading a working keyword highlighter for a mis-fitting stateful one.

| Lexilla lexer | PrimeEdit languages (77) |
|---|---|
| `cpp` | c, cpp, csharp, java, javascript, typescript, go, swift, kotlin, scala, objectivec, objectivecpp, cuda, glsl, groovy, solidity, protobuf, wgsl, v |
| `hypertext` | html, php, vue, svelte (embedded CSS: accepted gap) |
| `xml`, `json`, `yaml`, `toml`, `markdown`, `latex`, `diff` | xml, json, yaml, toml, markdown, latex, diff |
| `css` | css, less, scss |
| `props` | ini, dotenv, editorconfig, systemd |
| `conf` | apache |
| `python` | python, starlark |
| `lisp` | lisp, clojure, scheme |
| `sql`, `mysql` | sql, postgresql; mysql |
| one lexer each | ada, asm (assembly), bash, batch, caml (ocaml), cmake, COBOL (cobol), coffeescript, d, dart, erlang, fortran, fsharp, haskell, julia, lua, makefile, matlab, nim, nix, pascal, perl, powershell, r, ruby, rust, tcl, vb (visualbasic), verilog, vhdl, zig |
| **JSON rules (19)** | awk, crystal, csv, dockerfile, elixir, fish, git, graphql, hcl, jsonnet, meson, nginx, pkl, prolog, restructuredtext, sass, terraform, vim, wasm |

The lexer names were checked against the Lexilla 5.5.5 `LexerModule` declarations in the spike's extracted source.

- **JsonRuleLexer** ports `SyntaxHighlighter::loadFromDefinition`/`highlightBlock` (`highlighter.cpp:50-429`) to `ILexer5::Lex`:
  - the regexes run per line on `QString`, and match offsets map back to bytes;
  - multi-line comment and string state is carried in `SetLineState`;
  - `Fold` ports today's heuristic: indent folding for python, yaml, coffeescript, nim and haskell (`editor.cpp:1826-1831`), and brace folding otherwise (`editor.cpp:1875-1970`);
  - semantic style ids 1–11 are fixed (§4.4).
- **ThemeStyleMap.** `STYLE_DEFAULT` takes the theme background and foreground and the Settings font, followed by `SCI_STYLECLEARALL`. Then each lexer's style ids map to the 11 semantic classes of `highlighter.cpp:15-35`: keyword (bold), type, function, string, number, comment (italic), preprocessor (`variable` colour), operator, tag (`keyword` colour), attribute (`variable`), variable. The per-lexer style tables are data in `themestylemap.cpp`. The map also sets the caret-line, selection and whitespace elements, `STYLE_INDENTGUIDE`, `STYLE_BRACELIGHT`/`BRACEBAD`, `STYLE_LINENUMBER`, the fold-margin colour, marker colours and indicator colours from the matching `Theme` fields (`theme.h:16-65`). It is re-applied on `ThemeManager::themeChanged` and `Settings::fontChanged`.
- **Arabic fallback fonts.** Today `setupEditor` lists DejaVu Sans Mono, Liberation Mono, Courier New, Noto Sans Arabic and Amiri after the user's font (`editor.cpp:241-245`). Scintilla passes a single family name to `QFont`, so the same order is registered with `QFont::insertSubstitutions(<primary family>, …)`. **[UNVERIFIED: that PlatQt's `QFont` honours the substitution table. SB3 checks it with a screenshot of Arabic text in the receipt. If it does not, the regression is added to §3.7.2.]**

### 3.7 Editing features

#### 3.7.1 Mapping (every current feature)

| # | Today (where) | After S (mechanism) | Owner |
|---|---|---|---|
| 1 | Typing, undo/redo, cut/copy/paste, select all | native; `SCI_SETPASTECONVERTENDINGS(true)` | SB2 |
| 2 | Line numbers, `LineNumberArea` (`editor.cpp:342-510`) | margin 0, `SC_MARGIN_NUMBER`, width from `SCI_TEXTWIDTH` | SB2 |
| 3 | Bookmarks: toggle, next, previous, clear; `QSet<int>` (`editor.cpp:512-549`) | marker `MARK_BOOKMARK` in margin 1; `SCI_MARKERNEXT`/`MARKERPREVIOUS`. `bookmarks()` is derived from the markers. Markers move with their lines, where today's `QSet` does not: an improvement. | SB2 |
| 4 | Git gutter painted in the line-number area (`editor.cpp:346-440`) | markers `MARK_GIT_*` in margin 2; same `GitGutter` and timer, fed `rawText()`; Small and Medium only | SB2 |
| 5 | Fold margin and heuristic folding (`editor.cpp:1818-2116`) | margin 3, `SC_MASK_FOLDERS`; folding by the lexer or JsonRuleLexer; `SCI_SETAUTOMATICFOLD(SHOW\|CLICK\|CHANGE)`. `foldAll/unfoldAll` → `SCI_FOLDALL`. `foldAt/unfoldAt/toggleFoldAt` → `SCI_FOLDLINE`, after `SCI_COLOURISE(0, end of line)` so fold levels exist. `isFoldableLine` → `SC_FOLDLEVELHEADERFLAG`; `isFoldedLine` → header and not expanded; `foldedRegions()` is derived on demand. | SB2 |
| 6 | Word wrap | `SCI_SETWRAPMODE(SC_WRAP_WORD)` | SB2 |
| 7 | Tab width, insert spaces, Tab and Shift+Tab indent (`editor.cpp:874-912`) | `SCI_SETTABWIDTH`, `SETUSETABS`, `SETTABINDENTS`, `SETBACKSPACEUNINDENTS`; native Tab and BackTab, after the snippet attempt (#27) | SB2 |
| 8 | Zoom in/out/reset, Ctrl+wheel (`editor.cpp:578-608, 960-972`) | `SCI_SETZOOM`. The `wheelEvent` override keeps `m_zoomLevel` consistent. | SB2 |
| 9 | Go to line (centres; reveals folds) (`editor.cpp:610-621`) | `SCI_ENSUREVISIBLEENFORCEPOLICY` + `SCI_GOTOLINE` + `SCI_VERTICALCENTRECARET` | SB2 |
| 10 | `currentLine()` 1-based; `currentColumn()` UTF-16 + 1 (`editor.cpp:623-631`) | `SCI_LINEFROMPOSITION`; `SCI_COUNTCODEUNITS(lineStart, caret) + 1`. Same values. | SB2 |
| 11 | `findNext`/`findPrevious` with wrap; `replaceAll` reveals folds (`editor.cpp:633-681`) | `SCI_SEARCHINTARGET` with `SCFIND_MATCHCASE`/`WHOLEWORD` from the same `QTextDocument::FindFlags` parameter; target reset for the wrap; `replaceAll` inside one undo action, each match revealed. Whole-word uses Scintilla's word characters (alphanumerics, `_`, bytes ≥ 0x80): a small difference from Qt's Unicode boundaries. | SB2 |
| 12 | Mark All / clear / count; `markAll(pattern, opts, from, to)` limits marks to `[from, to)` (npp-qw1, `editor.cpp:1243-1280`) | `SearchEngine` results (UTF-16 line and column) → byte ranges, filtered to `[from, to)` (UTF-16, converted per §4.2) → `IND_FIND_MARK` | SB2 |
| 13 | `selectedText`, `replaceSelection` | `SCI_GETSELTEXT` → `QString`; `SCI_REPLACESEL`. Line breaks come back as LF, not U+2029 (§3.7.2 C2). | SB2 |
| 14 | Column selection: Alt+drag, type/delete/paste in columns, Escape (`editor.cpp:714-737, 2565-2790`) | native rectangular selection; `SCI_SETVIRTUALSPACEOPTIONS(SCVS_RECTANGULARSELECTION)` (Notepad++'s default; spike §7); `SCI_SETMULTIPASTE(SC_MULTIPASTE_EACH)`; native Alt+Shift+arrows. `pasteAtColumn` (one clipboard line per row) is ported over the selections. | SB2 |
| 15 | Column Editor: insert text or numbers (`editor.cpp:2667-2721`) | iterate `SCI_GETSELECTIONS` and the carets and anchors, inside one undo action | SB2 |
| 16 | Multi-cursor: Alt+click adds a caret, Ctrl+D next, Ctrl+Shift+L all, Escape, typing in every caret, extra carets painted (`editor.cpp:747-775, 2138-2381`) | `SCI_SETMULTIPLESELECTION(true)`, `SETADDITIONALSELECTIONTYPING(true)`; Ctrl+D → `SCI_MULTIPLESELECTADDNEXT`, Ctrl+Shift+L → `SCI_MULTIPLESELECTADDEACH` (target whole document, current find flags); Escape → keep the main selection. Alt+click without drag is intercepted in `mouseReleaseEvent` → `SCI_ADDSELECTION(pos, pos)`. Today's bindings are kept (Q4). | SB2 |
| 17 | `setLanguage` (accepts display names such as "C++" and ids, as `SyntaxHighlighter::loadLanguageRules` does, `highlighter.cpp:91-143`) | `LexerCatalog::apply`: in SB2 the lexer, `fold=1` and default styles, so folding works (R1-5); SB3 adds keyword sets, JsonRuleLexer and `ThemeStyleMap::apply` | SB2, then SB3 |
| 18 | LSP diagnostics: wave underline and tooltip (`editor.cpp:2798-2841`) | `IND_DIAG_ERROR..HINT` (`INDIC_SQUIGGLE`, same colours as today); the message shown as a call tip on `SCN_DWELLSTART` over a diagnostic. **[UNVERIFIED: whether `QPlainTextEdit` shows the `QTextCharFormat` tooltip today. Parity or better either way.]** | SB2 |
| 19 | LSP completion popup at `cursorRect` (`editor.cpp:2843-2850`) | same `CompletionPopup` placed at `SCI_POINTXFROMPOSITION`/`POINTYFROMPOSITION` | SB2 |
| 20 | `requestHover` (no caller, §2.3) | kept unchanged (it does not touch the widget); still unreachable | — |
| 21 | Ctrl+click and menu go-to-definition (`editor.cpp:2556-2563`, `mainwindow_extended.cpp:796`) | `mousePressEvent` override reads `event->modifiers()`, then `SCI_POSITIONFROMPOINT` → UTF-16 line and column | SB2 |
| 22 | `requestLSPCompletion` | position conversion only | SB2 |
| 23 | Sync API | §3.2 | SB2 |
| 24 | Duplicate, delete, move line up/down (`editor.cpp:1399-1499`) | `SCI_SELECTIONDUPLICATE`/`LINEDUPLICATE`, `LINEDELETE`, `MOVESELECTEDLINESUP`/`DOWN` | SB2 |
| 25 | Toggle comment, sort, remove duplicates, trim, join, case conversions; the npp-qw1 transforms (`applyTextTransform`, `scopeRange`, `textInRange`, `replaceRange`: `editor_transforms.cpp:12-58, 131-152`) | `applyTextTransform` → **`replaceRange`**: `SCI_BEGINUNDOACTION`, `SCI_SETTARGETRANGE`, `SCI_REPLACETARGET`, `SCI_ENDUNDOACTION`. `textInRange` → `SCI_GETTEXTRANGEFULL`. Positions at the adapter stay UTF-16 (§4.2). This is the only replacement point. | SB2 |
| 26 | Show whitespace, EOL, indent guides (painted, `editor.cpp:2383-2552`) | `SCI_SETVIEWWS(SCWS_VISIBLEALWAYS)`, `SCI_SETVIEWEOL`, `SCI_SETINDENTATIONGUIDES(SC_IV_LOOKBOTH)`. EOL glyph: §3.7.2 C3. | SB2 |
| 27 | Auto-close brackets and quotes, skip-over, Backspace deletes an empty pair, not inside strings (`editor.cpp:805-862, 1086-1117`) | `SCN_CHARADDED` handler plus a Backspace check in `keyPressEvent`; same pair table and text heuristic | SB2 |
| 28 | Bracket match highlight; jump to matching bracket (`editor.cpp:1000-1084, 1282-1316`) | `SCI_BRACEMATCH`, `BRACEHIGHLIGHT`/`BRACEBADLIGHT` on `SCN_UPDATEUI`; jump → `SCI_GOTOPOS(match)` | SB2 |
| 29 | Current-line highlight | `SC_ELEMENT_CARET_LINE_BACK` | SB3 |
| 30 | Auto-indent on Enter (`editor.cpp:914-932`) | `SCN_CHARADDED('\n')` inserts the previous line's indentation | SB2 |
| 31 | Word, path and LSP completion; snippets on Tab (`editor.cpp:2943-3369`) | logic kept; the current word and line read through `textInRange` | SB2 |
| 32 | Macro record (`editor.cpp:802`) and playback by `sendEvent` (`macrorecorder.cpp:103-138`) | Recording is unchanged; `macros.json` is unchanged. Umar's probe (Qt 6.2.4, offscreen, a plain `QWidget`) shows a non-spontaneous `sendEvent(Ctrl+End)` sets `QApplication::keyboardModifiers()` (§5.1). If `test_s0_probe`'s `sendEvent` case also passes on a real `ScintillaEdit`, playback needs nothing. **`KeyChordDispatcher` is built only if that case fails** (R1-12). | SB0 probe; SB2 only if needed |
| 33 | Read-only mirror, keyboard navigation while read-only (`editor.cpp:158-187`) | buffer read-only (§3.2); native navigation and selection | SB2 |
| 34 | Large-file viewport (dead, §2.3) | deleted; replaced by §3.5 | SB2 |
| 35 | Arabic per-block RTL pass (`editor.cpp:299-340`) | deleted (accepted bidi regression R1) | SB2 |
| 36 | Rainbow brackets (unreachable, §2.3) | deleted, with its 95 lines (`editor.cpp:1124-1218`) | SB2 |
| 37 | Fonts and settings signals | `SCI_STYLESETFONT`/`SIZE` from Settings; re-applied on `fontChanged`/`tabSettingsChanged` | SB3 |
| 39 | `goToLineColumn(line, column)`; `goToPosition(int)` (npp-qw1 `-n -c -p`, `editor_transforms.cpp:105-124`) | column and position UTF-16 → byte (§4.2), clamped to the line or document as today; `SCI_ENSUREVISIBLEENFORCEPOLICY`, `SCI_GOTOPOS`, `SCI_VERTICALCENTRECARET` | SB2 |
| 40 | `insertBlankLine(above)` (`editor_transforms.cpp:60-70`) | `replaceRange` at the line start or end (`SCI_POSITIONFROMLINE`, `SCI_GETLINEENDPOSITION`), then the caret on the new line | SB2 |
| 41 | `setBookmarks(const QSet<int>&)` (1-based; bookmark line operations, `editor_transforms.cpp:72-76`) | `SCI_MARKERDELETEALL(MARK_BOOKMARK)`, then `SCI_MARKERADD` per line | SB2 |
| 42 | `selectWordIfNoSelection()` (Select and Find Next, `editor_transforms.cpp:80-89`) | `SCI_WORDSTARTPOSITION`/`WORDENDPOSITION(caret, true)` → `SCI_SETSEL`; returns `textInRange` | SB2 |
| 43 | `selectBetweenMatchingBraces()` (`editor_transforms.cpp:91-103`) | unchanged logic over `rawText()` and `cursorPosition()` (`TextTransforms::matchingBrace`, UTF-16), then `select(start, end)` | SB2 |
| 44 | `commentSyntax()` → `m_highlighter->commentSyntax()` (`editor_transforms.cpp:126-129`, filled from the JSON definition at `highlighter.cpp:418-419`) | **`LexerCatalog::commentSyntax(language)`** reads `singleLineComment`, `multiLineCommentStart` and `multiLineCommentEnd` from the same `resources/syntax/*.json` definition, resolving the language the same way. Single owner: **SB2** (R1-4, R1-5), so `test_npp_qw1:513-522` passes before SB3 deletes `SyntaxHighlighter`. | SB2 |
| 45 | `mainwindow_npp_find.cpp:24,46,61,129`, `mainwindow_npp_format.cpp:71,84`, `mainwindow_npp_tools.cpp:54,98,119` use `textCursor()`/`insertPlainText` on the Editor | Phase A moves them to the facade (`hasSelection`, `selectionRange`, `insertText`). The result viewer at `mainwindow_npp_tools.cpp:147` is its own `QPlainTextEdit` and stays. | SA1 |
| 38 | Drop a file onto the editor opens it (MainWindow `dragEnterEvent`/`dropEvent`, `mainwindow.cpp:1250-1267`) | `Editor::dropEvent` forwards URL drops to the window and leaves text drops to Scintilla. **[UNVERIFIED: how `ScintillaEditBase` treats URL drops. SB2 tests both kinds.]** | SB2 |

#### 3.7.2 Regressions and behaviour changes (explicit)

| ID | Change | Status |
|---|---|---|
| R1 | Mixed Arabic/English: no per-block RTL; overlap where lexer style runs split an RTL run (spike §8) | accepted by the owner |
| R2 | Files of 20–50 MiB lose heuristic folding (Medium uses `STYLES_NONE`, no lexer) | proposed; the owner accepts at the gate or the cut-over (R1-19d); follow-up F7 |
| R3 | A file under 50 MiB with a line over `L` (for example a 2 MB minified JSON) opens read-only with wrapped lines. Today it opens editable (slowly) | proposed; Q1 |
| R4 | Embedded CSS in HTML, Vue and Svelte is not highlighted | proposed as an accepted gap |
| R5 | `L` is enforced only at load: a line longer than `L` that enters by paste, join or draft restore stays editable and may be slow (measured in SB6, §3.5.3) | proposed known limit; F22 |
| C1 | `toPlainText()` returns the raw text: U+00A0 no longer reads as a space in Find, Function List, LSP and the plugin API (`EditorAPI::currentText`, a public interface, §3.8.7) | intended (fidelity) |
| C2 | `selectedText()` returns LF, not U+2029, including through `EditorAPI::selectedText` (§3.8.7) | intended |
| C3 | Show EOL draws Scintilla's EOL marker instead of `¶`. Because the buffer is LF-only, SB2 sets the `"\n"` representation to the document's ending ("CRLF", "CR" or "LF"). **[UNVERIFIED: that 5.6.9 applies representations to line ends; if not, the marker reads "LF" and the status bar shows the real ending.]** | intended |
| C4 | Fold extent follows the lexer. With C++, the closing-brace line folds with the body, as in Notepad++ | intended |
| C5 | Bookmarks and git-gutter marks move with their lines while editing | improvement |
| C6 | Undo survives Move to Other View | improvement |
| C7 | A Medium file (20–50 MiB) with lines between 8,000 characters and `L` opens editable; today it opens read-only (`document.cpp:111-121`) | intended (R1-19c) |

### 3.8 Integration

#### 3.8.1 LSP (incremental sync)

- **`LspChangeTracker`** is created per document when `didOpen` is sent. It connects to `TextBuffer::changed(BufferChange)`. This is document level, so one stream serves every view, including both panes. `DocWatcher` receives `BeforeDelete` without any event mask (`Document.cxx:1492`).
  - `BeforeDelete` records the start and end as (line, UTF-16 column) through `TextBuffer::lineFromPosition` and `countUtf16(lineStart, pos)` (internally `Document::CountUTF16`, the function behind `SCI_COUNTCODEUNITS`, `Editor.cxx:9141`).
  - `DeleteText` queues that range with empty text.
  - `InsertText` queues an empty range at the insert position, with `QString::fromUtf8(text, len)`.
  - Notifications arrive synchronously inside the edit, so the tracker only appends. It does no I/O and never re-enters the buffer for writes.
- **`LSPClient`** reads `capabilities.textDocumentSync` (a number, or an object's `change`) from the initialize result: 2 is incremental, 1 is full (today's behaviour), 0 or absent sends no `didChange`. Position encoding stays UTF-16, the LSP default. The client advertises nothing else. New pure static `LSPClient::didChangeParams(uri, version, changes)` builds the JSON, so it can be tested without a server.
- **`LSPBridge`** keeps its 300 ms debounce (`lspbridge.cpp:17-20`). On the timer it flushes **every** tracker with pending changes, not only the active editor's: a replace-all across open documents (npp-qw1) changes background documents too. It also flushes before every request (completion, definition), so positions line up. A buffer replacement (reload) sends one full-text change with no range, which incremental mode allows.
- **No LSP for LargeFile documents.** Today they would send a 2 MB preview as if it were the file.

#### 3.8.2 Git gutter

`GitGutter` (`gitgutter.cpp:24-60`) is unchanged and still runs on the 1 s debounce with `rawText()`. The Editor turns `changes()` into `MARK_GIT_ADDED`/`MODIFIED`/`DELETED` markers in margin 2, using today's colours. It runs on Small and Medium documents only, not LargeFile: a git diff of a 500 MB view is pointless.

#### 3.8.3 Minimap

- `DocumentMapWidget` becomes a `ScintillaEdit` attached to the active editor's buffer with `SCI_SETDOCPOINTER`.
- Settings: `SCI_SETZOOM(-10)`, no margins, no caret, no scrollbars, `Qt::NoFocus`.
- The widget consumes key events and does **not** call `SCI_SETREADONLY` (document-wide, §3.2).
- A semi-transparent rectangle painted over it marks the main view's first visible line and lines on screen. Clicking or dragging scrolls the main editor (`SCI_SETFIRSTVISIBLELINE`).
- It re-attaches on tab change and on `bufferReplaced`.
- It replaces the line-length bars painted from `QTextBlock` (`documentmap.cpp:59-180`).

#### 3.8.4 Markdown mode

`markdownmode.cpp:90-94,160` already calls only the facade after Phase A (`textChanged`, `cursorPositionChanged`, `currentLine`, `toPlainText`). The preview overlay stays a child widget of the Editor. Phase B only checks that the overlay geometry follows `ScintillaEditBase`'s viewport (`tst_markdown`).

#### 3.8.5 Split views and session

- Move to Other View creates the new `Editor` on the same `Document`, so the buffer and undo are kept, and the old one is closed. The D5 lifetime guard is unchanged.
- Session fold state: header line numbers, applied with `foldAt` after colourising (#5). Bookmarks: line numbers. Draft entries: unchanged keys, with `cursorPosition` in UTF-16 units and `scrollPosition` as the first visible line. These are the same meanings as today (verified: nothing reads either key back).

#### 3.8.6 Clone to Other View — **out (F2)**

The architecture makes it possible: two `Editor`s on one buffer. But it reopens D5, Phase 1's "a Document is shown by at most one editor" (`2026-11-26` §3 D5):

- the draft writer iterates `allEditors()` and would cache a cloned document twice;
- the close lifetime guard and close prompt would need per-document rules;
- the reload bar would apply to two widgets.

Each of those touches I5 or D5. It is not a feature today (Phase 1 removed it). It ships as follow-up F2 with its own spec.

#### 3.8.7 Plugin API

`EditorAPI` is a public interface (`src/core/plugininterface.h:19-48`), and its semantics are kept. `cursorPosition`/`setCursorPosition` stay UTF-16 offsets, `cursorColumn` stays UTF-16 + 1, and `setText`/`insertText` go through the facade (`mainwindow.cpp:2396-2464`). No signature changes.

**Two returned values change for plugins (R1-15).** `currentText()` and `selectedText()` (`mainwindow.cpp:2401-2426`) now return the raw text: U+00A0 and U+2028 come back as themselves instead of as a space and a line feed (C1), and line breaks inside a selection come back as LF instead of U+2029 (C2). This is a behaviour change on a public interface. It is stated here, and SB6 lists it in `RELEASE_NOTES.md`.

#### 3.8.8 Automatic whole-text consumers on LargeFile documents

The function list refresh, the Markdown outline and preview, the git gutter, LSP and word-completion scanning would each build a `QString` of up to 2× the file. They are off for LargeFile documents. Find All, Count and Mark stay available on demand. Their transient cost is 2× the view size plus the results, and it is documented in `html-docs/guide.html#largefile`. Making search buffer-based is F1.

**Status bar on every caret move (R1-9).** `StatusBarWidget::updateFromEditor` (`statusbar.cpp:139-141`) reads `selectedText().length()` and `textCursor().position()` on each caret move. After S it calls `selectionLength()` and `cursorPosition()` (§4.2). Both are O(log lines + line length) through the UTF-16 line-character index, which is allocated on **every** tier, LargeFile included. So the status bar never builds a `QString` of the selection, and never scans the file from the start. Select All on a 500 MB view therefore allocates nothing. S4 cases 7–8 measure it.

### 3.9 Build and packaging

- **Vendor, do not fetch.** Decision: commit the sources under `third_party/`, pinned by tarball sha256.
  - Builds stay offline and reproducible: CI, a `.deb` build, a build at a café. Build-time fetching adds a network dependency to every configure, and a single point of failure (scintilla.org).
  - It also hides the upstream code from review.
  - Size: 4.6 MB (the needed subtrees: Scintilla `include/ src/ qt/` = 1.9 MB, Lexilla `include/ lexlib/ lexers/ src/Lexilla.cxx` = 2.7 MB, measured).
  - `ScintillaEdit.{h,cpp}` are committed already generated, so Python is not a build dependency.
- **Pin and integrity.**
  - `third_party/SCINTILLA_PIN` records the URLs, tarball sha256s (`1622546c…dcacb`, `fa4093ad…f229`, spike §2), versions, the subtree list, the generator command and any per-file sanitizer exception (below).
  - `tools/vendor_scintilla.sh <download-dir>` reproduces the tree from the tarballs. It reuses `fetch_sources.sh`'s download-and-verify step, runs `python3 -I WidgetGen.py`, copies the subset and writes `MANIFEST.sha256`.
  - The CTest `vendor_manifest` runs `sha256sum -c third_party/MANIFEST.sha256` offline. Any edit to vendored code fails CI.
  - Policy: no local patches. Bumps are deliberate: re-run the script, then commit.
- **CMake.** `cmake/Scintilla.cmake` mirrors the spike's `CMakeLists.txt`:
  - `scintilla_qt` (33 core files plus the Qt layer, AUTOMOC);
  - `lexilla` (all 125 lexers; `CreateLexer` called directly);
  - both STATIC, with `-w` on their sources only;
  - `SCINTILLA_QT=1` public, `MAKING_LIBRARY=1` private;
  - linked PUBLIC into `prime-edit-core`.
- **`libqt6core5compat6`: Depends, no patch.**
  - `ScintillaQt.cpp` and `PlatQt.cpp` use `QTextCodec` at 13 sites (grep). With the code page fixed at UTF-8 (B1) none of them runs, but all of them link.
  - Patching them out is a local fork to rebase on every bump (spike risk 8).
  - The package is in Ubuntu main for 22.04 at 6.2.4-1 and is 421 kB installed (`apt-cache`). The dev package on 22.04 is `libqt6core5compat6-dev`. **[UNVERIFIED: the noble (CI) dev package name. SB0 checks with `apt-cache policy` on the runner, and `qt6-5compat-dev` is the candidate.]**
- **`qt6-wayland`: not added.** Nothing in this programme needs it: today PrimeEdit runs through XWayland, and so does the migrated build. Adding it (even as Recommends, which apt installs by default) would move every Wayland user to the native platform. That has two problems: IME was verified only on nested Weston (spike §4), and the D4 single-instance raise needs an activation token on Wayland. Native Wayland is follow-up F21, with a raise test.
- **Packaging files.**
  - `BUILD.md`: prerequisites (`libqt6core5compat6-dev`) and the control snippet (`BUILD.md:94`) become `Depends: libqt6core6, libqt6gui6, libqt6widgets6, libqt6core5compat6, libicu70`. No Recommends line.
  - The Scintilla and Lexilla `License.txt` files are installed under `share/doc/prime-edit/` (HPND notice requirement).
  - CI's apt lines (`.github/workflows/build.yml`) gain the Core5Compat dev package.
- **Sanitizers.** The ASan/UBSan job compiles the vendored code with the same flags. Zero reports remains the gate. A report inside `third_party/` is reported upstream, and gets a `-fno-sanitize=<check>` on that one source file only, recorded with its reason in `SCINTILLA_PIN`. It is never suppressed globally.

### 3.10 Threading and performance

Everything stays on the GUI thread, as in Phase 1. No new threads.

- `SCI_SETLAYOUTTHREADS` stays at its default (the spike measured no gain).
- Small documents use `SCI_SETIDLESTYLING(SC_IDLESTYLING_TOVISIBLE)`.
- Loading a 500 MB view blocks for about 2.5 s, against 0.58 s for today's preview. A busy cursor is shown during `openFile`.
- Saves are unchanged: synchronous and bounded at 50 MiB, because only editable documents save.

## 4. Contracts

Every identifier here crosses a workstream boundary, so it is fixed here before dispatch.

### 4.1 Core (SB1 → SB2, SB4, SB5)

| Contract | Notes |
|---|---|
| `class TextBuffer : public QObject` · `static std::unique_ptr<TextBuffer> fromUtf8(const QByteArray &, BufferOptions)` · `void *docPointer() const` · `qint64 length() const` · `const char *bufferPointer()` · `const char *rangePointer(qint64 pos, qint64 len)` · `void setSavePoint()` · `bool isSavePoint() const` · `void setReadOnly(bool)` · `bool isReadOnly() const` · `qint64 lineFromPosition(qint64) const` · `qint64 lineStart(qint64 line) const` · `qint64 countUtf16(qint64 from, qint64 to) const` · `void replaceAll(const QByteArray &utf8)` · `void insertUtf8(qint64 pos, const QByteArray &)` · `void deleteRange(qint64 pos, qint64 len)` · `bool undo()` · `bool redo()` · `bool canUndo() const` · `bool canRedo() const` | Byte positions, 64-bit. `bufferPointer` is valid until the next modification (B2). `replaceAll`/`insertUtf8` are one undo action each. |
| `enum class BufferOptions { Styled, StylesNone }` | `StylesNone` adds `TEXT_LARGE` |
| signals `TextBuffer::savePointChanged(bool atSavePoint)`, `TextBuffer::changed(const BufferChange &)` | `struct BufferChange { enum Kind { BeforeDelete, Deleted, Inserted } kind; qint64 position; qint64 length; QByteArray text; /* Inserted only */ qint64 linesAdded; bool undoRedo; }` |
| `TextBuffer *Document::buffer() const` · signal `Document::bufferReplaced()` | A new buffer after every `load` |
| `enum class ViewKind { Editable, LargeView, LongLineView, Preview }` · `ViewKind Document::viewKind() const` | `Editable` for Small and Medium; the others are LargeFile, and `isPartialView()` is true for them |
| `static void Document::setMemoryBudgetForTest(qint64 bytes)` · `static void Document::setLongLineThresholdForTest(qint64 bytes)` · `static void Document::setUtf16CapForTest(qint64 units)` | −1 restores the default. `setSizeThresholdsForTest` is unchanged. |
| `bool Document::isEmpty() const` | buffer length 0 |
| `quint64 TextBuffer::changeCount() const` | incremented by the `DocWatcher` on every modification; the B2 debug assertion compares it (R1-1) |
| `qint64 TextBuffer::utf16FromPosition(qint64 bytePos) const` · `qint64 TextBuffer::positionFromUtf16(qint64 utf16) const` · `qint64 TextBuffer::utf16Length() const` | Through the UTF-16 line-character index (`Document::AllocateLineCharacterIndex`), allocated when every buffer is built, whatever the tier. O(log lines + line length). Lines are bounded: ≤ `L` when editable, ≤ 4,096 bytes when reflowed (R1-9). |
| `void Document::finishSave(const QString &path, const QByteArray &sha1, qint64 size)` (private) | replaces `finishSave(path, written)`; called only after the view is dropped (B2) |
| `void Document::replaceContent(const QString &)` | **Phase A (SA2)**: the PieceTable body. SB1: `TextBuffer::replaceAll`. It never touches a modified flag. Used only by `Editor::syncToDocument`, until SB2 makes that a no-op (§3.2 bridge). |
| Unchanged signatures | `Document::save`, `saveAs`, `writeAtomic`, `writeDirect`, `text`, `setText`, `insert`, `remove`, `undo`, `redo`, `isModified`, `setModified`, `isPartialView`, `isMinified`, `fileMode`, `setSizeThresholdsForTest`; `SaveResult`; `WriteMode`; `DiskState` |
| Removed | `Document::content()`, the `PieceTable` member, `largeFileReader()`, `isLargeFile()` (always false today; the only callers are the dead Editor viewport paths) |
| `struct LspContentChange { bool full; int startLine, startChar, endLine, endChar; QString text; }` · `class LspChangeTracker { explicit LspChangeTracker(TextBuffer *); QVector<LspContentChange> takePending(); bool hasPending() const; }` | SB4 internal, consumed by LSPBridge |
| `enum class LspSyncKind { None, Full, Incremental }` · `LspSyncKind LSPClient::syncKind() const` · `void LSPClient::didChange(const QString &uri, const QVector<LspContentChange> &)` · `static QJsonObject LSPClient::didChangeParams(const QString &uri, int version, const QVector<LspContentChange> &)` | the old `didChange(uri, QString)` stays for Full |

### 4.2 Editor facade (SA1 on `QPlainTextEdit` → SB2 on `ScintillaEdit`; consumed by SA2, SA3, SB4, SB5)

Positions and columns are UTF-16 code units and lines are 1-based, exactly as `textCursor()` gives them today. Byte positions never leave `Editor`.

| Member | Phase A body (QPlainTextEdit) | Phase B body |
|---|---|---|
| signals `textChanged()`, `cursorPositionChanged()`, `selectionChanged()`, `undoAvailable(bool)`, `redoAvailable(bool)` | inherited | emitted from `SCN_MODIFIED` / `SCN_UPDATEUI` |
| `QString rawText() const`, `QString toPlainText() const` | existing; `toPlainText` inherited | both return the buffer as `QString` (C1) |
| `int cursorPosition() const`, `void setCursorPosition(int)` | `textCursor().position()` | UTF-16 ↔ byte conversion (below) |
| `bool hasSelection() const`, `QPair<int,int> selectionRange() const`, `void select(int start, int end)` | `QTextCursor` | `SCI_GETSELECTIONSTART`/`END`, converted |
| `void insertText(const QString &)` | `insertPlainText` | `SCI_REPLACESEL` |
| `bool isEmpty() const` | `QTextDocument::isEmpty` | `length == 0` |
| `bool isContentModified() const` | `QTextDocument::isModified` | `!isSavePoint() \|\| draftDirty` |
| `bool canUndo() const`, `bool canRedo() const` | `QTextDocument` | `SCI_CANUNDO`/`CANREDO` |
| `int lineCount() const`, `int lineLength(int line) const` (UTF-16, without EOL), `int firstVisibleLine() const`, `void setFirstVisibleLine(int)`, `int linesOnScreen() const` | blocks and scrollbar | `SCI_GETLINECOUNT`, `COUNTCODEUNITS`, `GETFIRSTVISIBLELINE`, `LINESONSCREEN` |
| `QRect caretRect() const`, `void ensureCaretVisible()` | `cursorRect`, `ensureCursorVisible` | `POINTX/YFROMPOSITION`, `SCI_SCROLLCARET` |
| `int selectionLength() const` (UTF-16 units; for the status bar, R1-9) | `textCursor().selectedText().length()` | `utf16FromPosition(end) − utf16FromPosition(start)`: no allocation |
| `bool isReadOnly() const` | inherited | buffer flag |
| npp-qw1 members (`editor.h:121-137`, regenerated against `ce5d206`, R1-4) | `applyTextTransform`, `textInRange`, `replaceRange`, `scopeRange` (private), `insertBlankLine`, `setBookmarks`, `selectWordIfNoSelection`, `selectBetweenMatchingBraces`, `goToLineColumn`, `goToPosition`, `commentSyntax`, `markAll(…, from, to)`: today's bodies (`editor_transforms.cpp`, `editor.cpp:1243-1280`) | §3.7.1 rows 12, 25 and 39–44; positions UTF-16 |
| Unchanged | every other public method in `editor.h:38-198`, except `lineNumberAreaWidth`, `lineNumberAreaPaintEvent`, `bookmarkMarginWidth`, `foldMarginWidth` and the `using QPlainTextEdit::…` lines. Those are removed in SB2 (no caller outside `editor.cpp`/`linenumberarea.cpp`). | |

**UTF-16 ↔ byte conversion (R1-9).** Every buffer allocates the UTF-16 line-character index. A conversion finds the line through the index (O(log lines)), then counts within that line (O(line length); lines are bounded as in §4.1). There is no linear path from the start of the document on any tier. The index's memory and build time are part of the §3.5.2 factors, and SB6 measures them. Positions fit `int` because whole-file views are capped at UTF-16 length 2³¹ − 1 (§3.5.2).

### 4.3 Test support (SA3 → every suite)

`tests/regression/editor_driver.h`:

- `typeText(Editor*, const QString&)` (`QTest::keyClicks`);
- `keyChord(Editor*, Qt::Key, Qt::KeyboardModifiers)`;
- `moveTo(Editor*, int utf16)`;
- `selectRange(Editor*, int, int)`;
- `contentModified(Editor*)`;
- `undo(Editor*)`, `redo(Editor*)`.

`keyChord` uses `QTest::keyClick`. Umar's probe shows `QTest::keyClick` sets the global modifier state (§5.1). SB0's `test_s0_probe` confirms it on a real `ScintillaEdit` before any Phase B test relies on it. If it fails there, `keyChord` sends the SCI command for the chord instead. The ported suites use only the driver, the facade and the `Document` API.

### 4.4 Editor ids (`src/ui/editorids.h`; SB2 ↔ SB3 ↔ SB5)

| Kind | Ids |
|---|---|
| Margins | 0 line numbers · 1 bookmarks (16 px) · 2 git (3 px) · 3 fold (16 px). Widths from `editor.h:34-36`. |
| Markers | 0 `MARK_GIT_ADDED` · 1 `MARK_GIT_MODIFIED` · 2 `MARK_GIT_DELETED` · 3 `MARK_BOOKMARK` · 25–31 Scintilla fold markers |
| Indicators | 8 `IND_FIND_MARK` · 9–12 `IND_DIAG_ERROR`, `WARNING`, `INFO`, `HINT` · 13–31 reserved: 13 smart highlight (F3), 14–18 mark styles (F4) |
| JsonRuleLexer styles | 0 default · 1 keyword · 2 type · 3 function · 4 string · 5 number · 6 comment · 7 preprocessor · 8 operator · 9 tag · 10 attribute · 11 variable |

## 5. Test strategy

### 5.1 Harness facts

- `ScintillaEditBase::keyPressEvent` reads `QApplication::keyboardModifiers()` (`ScintillaEditBase.cpp:213,246-248`), not the event's modifiers.
- **Umar's probe** (`scratchpad/keyprobe/probe2.cpp`; Qt 6.2.4, offscreen, a plain `QWidget`, gate evidence) found both paths set the global modifier state:
  - `keyClick(Ctrl+End), keyClick(End): hits=2 global_ctrl=1 after=0`. `QTest::keyClick` sets the modifiers for the chord and clears them afterwards.
  - `sendEvent only: hits=1 global_ctrl=1`. A non-spontaneous `QApplication::sendEvent`, the `MacroRecorder` playback path, sets them too.

  This contradicts the premise behind `KeyChordDispatcher` in the round-1 draft, which came from a code comment and was never run (R1-12).
- **`test_s0_probe` (SB0's first commit)** repeats both cases on a real `ScintillaEdit` with 100 lines:
  - (a) `QTest::keyClick(Ctrl+End)`, then a plain `End`;
  - (b) `QApplication::sendEvent` of a `Ctrl+End` `QKeyEvent`.

  Each must leave the caret on the last line, and the plain `End` must not act as `Ctrl+End`. The output is recorded in the receipt, and both cases stay as regression cases.
- **Decision rule.** `KeyChordDispatcher` is built **only if case (b) fails**. If (b) passes, the dispatcher, `src/ui/keychords.*` and its table test are not written, and macro playback needs no special path. If (a) fails, `editor_driver.h`'s `keyChord` sends SCI commands (§4.3). S2 case 6 (macro playback) runs either way.

### 5.2 Rules

1. **Failing tests first, per workstream.** Declarations with placeholder bodies land first (Phase-1 W0 pattern). The new cases are committed, and their failing output is recorded in the receipt before any implementation.
2. **Ports change mechanics, never assertions (R1-10).** Every existing assertion line stays byte-identical, except those in the amendments table (§5.4). That covers `QCOMPARE`, `QVERIFY`, `QVERIFY2`, `QTRY_*` (6 uses today), `QFAIL`, `QSKIP` and `QEXPECT_FAIL`. New cases may add assertions. Three checks, each pasted into the receipt:
   - `git diff -U0 <base> -- tests/ | grep -E '^-.*\b(QCOMPARE|QVERIFY2?|QTRY_[A-Z_]+|QSKIP|QEXPECT_FAIL|QFAIL)\b'` lists exactly the amended lines (removed or changed);
   - `git diff -U0 <base> -- tests/ | grep -E '^\+.*\b(QSKIP|QEXPECT_FAIL)\b'` is empty: no skip or expected failure may be added;
   - for every existing suite, `<suite> -functions` run on the base build and on the task build gives identical listings (`diff`): no test function is deleted or renamed. New suites and new functions in a new case are listed separately.
3. **Phase A proves the ports on the old editor.** The 24 suites, ported to the facade and driver, pass on `QPlainTextEdit` before any Scintilla code exists. Phase B must then keep the same files green.
4. All 24 suites, plus the new `test_s*` suites and `vendor_manifest`, pass in Release, and pass under ASan/UBSan with 0 reports. **Timing assertions run only in non-sanitizer Release builds (R1-11).** SB0's CMake defines `PRIMEEDIT_TIMING_ASSERTS=1` when `CMAKE_BUILD_TYPE` is `Release` and `CMAKE_CXX_FLAGS` contains no `-fsanitize`. Without it, the timing cases (9a, 9b, S4 case 7) print their measurements and assert nothing about time. Every functional assertion in those cases still runs.

### 5.3 Files to port (Phase A, SA3)

`QTextCursor`/`QTextBlock`/`QPlainTextEdit` API is used in:

- regression: `test_i1_roundtrip`, `test_t1_2_flags`, `test_t1_5_cache`, `test_t2_1_modified`, `test_t2_2_sync`, `test_t2_3_folding`, `test_t2_4_readonly`, `test_t3_2_save_ui`, `test_t3_3_encoding_ui`, `test_t3_4_reload` (the 10 with `QTextCursor`/`QTextBlock`), plus `test_t1_10_session_autosave`, `test_t1_11b_drafts`, `test_t1_12_short_write`, `test_t3_1_split` (`toPlainText`/`setPlainText`/`textDocument`);
- validation: `crash_test`, `test_all_features`;
- feature: `tst_markdown`;
- npp-qw1: `test_npp_qw1` (62 matching lines).

### 5.4 Amendments (the only assertion changes allowed)

| ID | Suite :: case | Change | Why |
|---|---|---|---|
| A1 | `test_t2_1_modified::rtlPassLeavesLatinBlocksAlone` | The per-block `layoutDirection` loop is removed. `!isUndoAvailable()` becomes `!editor.canUndo()`. | No per-block direction exists after S (R1). Lands in SA3, because a `QTextBlock` loop cannot be written against the facade. |
| A2 | `test_t3_2_save_ui::partialViewCtrlSAndConversionsLeaveBytesUnchanged` (line 226) | Assert that the notice contains "read-only" instead of "part of". | With thresholds (1024, 4096) the file becomes a LargeView: whole, not partial. The notice names the view kind truthfully. Lands in SB5. |
| A3 | `test_t2_3_folding` cases that assert which line the caret reveals after entering a fold | Only if the lexer's fold extent (C4) moves the revealed line. The new expected line is cited from the lexer's fold levels in the receipt. | Lexer folding replaces the heuristic. `rawText()` assertions (D2) never change. |

Nothing else. A port that needs another assertion change stops and goes back to the orchestrator.

### 5.5 New suites (failing first), registered in `REGRESSION_SUITES`

| Suite | Owner | Cases |
|---|---|---|
| `test_s0_probe` | SB0 | the §5.1 probe |
| `test_s1_buffer` | SB1 | 1 `TextBuffer` built without a view: load, length, pointer, save point, read-only, undo. 2 Save fast path: UTF-8/LF writes the same bytes as `QByteArray(p, n)`; VmRSS delta under 1 MiB on a 40 MiB save (spike §6b). 3 `Utf8Scan::validate` equals `TextCodec::decode(…,"UTF-8").lossless` on a corpus: valid, overlong, surrogate `ED A0 80`, over U+10FFFF, truncated, each at a 16 MiB chunk edge. 4 Byte EOL normalisation and line-ending counts equal `adoptLineEndings`/`detectLineEnding` and `normalizeLineEndings`, for LF, CRLF, CR, mixed, lone CR at EOF, NUL beside a break (checked before normalisation, R1-18), an **empty file** and a **file with no line break** (both take `defaultLineEnding()`, D9, R1-3). 5 Save with CRLF, CR, a BOM, a stray `\r` in an LF buffer, and windows-1256: bytes equal today's `normalizeLineEndings` + `TextCodec::encode`. 6 B2 (R1-1): a slot connected to `modifiedChanged` and `saved` calls `setText("changed")` during the save. The file on disk and the emitted `sha1`/`size` equal the pre-save buffer, nothing reads a dangling pointer (ASan), and the debug `changeCount` assertion holds. 7 I2: type, then undo to clean; draft restore stays modified after type plus undo; a conversion stays modified after undo; undo past the save point, edit, and undo again never reads clean. 10 programmatic `setText` on a read-only document succeeds and sets modified. 18 `setText` then `undo` on a clean document reads clean (R1-17b). 19 `replaceContent` on a clean document leaves `isModified()` false (R1-17a). |
| `test_s1_load` | SB1 | 8a §5.2 at Document level (must fail on base). 9a `tests/100mb.json` at Document level (§3.5.3). 11 Tier selection at the thresholds and the budget hook. 12 Preview over budget: 2 MiB, notice. 13 Reflow never splits a code point (a 4-byte character straddling 4,096). 14 `setSizeThresholdsForTest(1024, 1 MiB)`: Medium is editable and round-trips UTF-16 (the existing `mediumModeDecodesUtf16`, re-asserted on the new path). 15 (R1-2) Bytes the detector calls UTF-8 but which fail strict validation (`C0 AF`, `ED A0 80`), and windows-1252 bytes reopened as UTF-8: each opens through the decode path, read-only and lossy, and the buffer is valid UTF-8. 16 (R1-2) Preview cut: a 4-byte UTF-8 character and a UTF-16LE surrogate pair each straddling the 2 MiB mark: the Preview ends before it, and the decode is lossless. 17 (R1-3) A cached draft whose original has a line over `L` (it opens as a LongLineView) lands in a writable untitled tab with `originalFilePath` set; the LongLineView tab shows the file. 20 (R1-9) A file whose UTF-16 length exceeds the cap (simulated through a test hook, `Document::setUtf16CapForTest`) opens as a Preview. |
| `test_s2_editor` | SB2 | 8b §5.2 at Editor level: Ctrl+End through MainWindow (must fail on base). 9b `tests/100mb.json` at Editor level. Then one case per row of §3.7.1 that has behaviour: rows 2–16, 18, 19, 21, 24–28, 30–33, 38. Includes 6 (macro playback: record "Ctrl+End, type x", play it back, and the text ends with x), and rows 39–44 (npp-qw1 members; `test_npp_qw1` also covers them). |
| `test_s2_highlight` | SB3 | Every `LanguageManager::availableLanguages()` entry resolves to one catalog entry with a non-null lexer. For 10 representative languages (cpp, python, html, json, yaml, bash, sql, markdown, css, and elixir through the JSON rules): a keyword, a comment and a string get the theme's colours. A theme switch re-styles. JsonRuleLexer multi-line comment state crosses lines. Indent folding for python-family languages through the JSON rules. |
| `test_s3_lsp` | SB4 | Shadow replay (spike §6a, extended): 30 scripted edits (emoji, Arabic, multi-cursor typing, undo/redo, `replaceRange`, column delete, a reload as full replace). The shadow equals the buffer after each flush. `didChangeParams` JSON shape and version increments. `syncKind` parsing for a number, an object and absence. Flush before a completion request. No tracker for a LargeFile document. |
| `test_s4_views` | SB5 | The minimap shares the buffer (an edit in the editor is visible in the minimap without a copy), never sets read-only, and clicking scrolls. Move to Other View keeps undo. Markdown overlay geometry. Status-bar column in UTF-16 on a line with emoji. Plugin API `cursorPosition` in UTF-16. View-kind notices for all three kinds. Automatic whole-text consumers are off for a LargeView (§3.8.8). 7 (R1-9) In a 60 MiB LargeView, after Ctrl+End, 20 caret moves each update the status bar in ≤ 10 ms (median; a timing case, §5.2 rule 4). 8 (R1-9) Select All in that view plus the status-bar update raises VmRSS by < 1 MiB. 9 (R1-3) Type, Move to Other View, undo: the document reads clean (D1: the constructor applies no `SyncTarget`). |

### 5.6 Receipts that are not CTests

- **SB6 measurement.** All in MiB and relative to **B**, measured in the same run as B (§1):
  - the RSS factors (§3.5.2), including the 100 MiB windows-1256 and UTF-16LE files and the UTF-16 index cost;
  - the `L` sweep (§3.5.3);
  - the 5 MiB-line draft restore (R5);
  - 500 MB and `100mb.json` open time, measured externally with the spike's `scripts/measure_open.sh`, against the 1.0.2 Release build;
  - startup time and idle RSS with 10 small files, on both builds.
- **Gating rows (R1-7).** G1 (500 MB view), G2 (`100mb.json`), G3 (save fast path), G4 (startup and 10 files) and the §3.5.2 factor rows. A miss of more than 15 % on any of them blocks the cut-over merge until a spec amendment passes the gate. Other measurements are reported, and a miss is flagged.
- **Timing asserts in CTest** (cases 9a, 9b and S4 case 7) use 2× the target as the hard limit, and run only under `PRIMEEDIT_TIMING_ASSERTS` (§5.2 rule 4). The receipt reports the measured number against the real target.
- **The P1.12 strace receipt and the P1.11 strace receipt**, re-run on the migrated build.

## 6. Blast radius

- **Code:**
  - `src/core/document.*` (load and the save byte source);
  - new `textbuffer`, `utf8scan` and `lspchangetracker` in `src/core/`;
  - `lspclient`;
  - `session.cpp` (facade calls);
  - all of `src/ui/editor*`;
  - deleted `linenumberarea.*` and `highlighter.*`;
  - `documentmap`, `markdownmode`, `lspbridge`, `mainwindow*.cpp` (157 references, mostly mechanical in Phase A), `statusbar`, `functionlist`;
  - `CMakeLists.txt`, CI, `BUILD.md`;
  - 18 test files ported.
- **Behaviour users will notice:** files over 50 MiB open whole (read-only), with the end reachable; long-line files open whole and wrapped (read-only, R3); new highlighting for 77 languages; Show EOL glyph (C3); bookmarks follow their lines (C5); no Arabic RTL layout (R1); no folding at 20–50 MiB (R2).
- **Data:** no change to any on-disk format (session, drafts, settings, macros). 1.0.2 and the new build read each other's files (rehearsed in SB6).
- **Dependencies:** `libqt6core5compat6` (Depends). The binary grows by Scintilla and Lexilla (measured in SB6).

## 7. Sequencing and rollback

### 7.1 Options

- **A build flag or runtime switch carrying both cores:** rejected. The two cores differ in who owns the text (PieceTable plus QTextDocument against one Scintilla document). Both would need two `Document` byte sources, two `Editor` implementations and every suite run twice. The old path rots behind the flag, and the save path, the riskiest part, would exist twice. That fails P1 and AP8.
- **Slices merged to master one by one:** not possible past Phase A. Once `Document` owns a `TextBuffer`, the `QPlainTextEdit` editor has no text to show.

### 7.2 Recommendation: one cut-over, prepared by a behaviour-preserving Phase A

1. **Phase A, on master.** The facade on `QPlainTextEdit` (SA1); Session and LSPBridge callers moved to the facade (SA2); the driver and the suite ports (SA3).
   - No behaviour change, and no Scintilla code.
   - Gate: 24 suites green in Release and ASan; the assertion diff equals A1 only; its own output gate.
   - It merges to master normally. It shrinks the cut-over diff and proves the ported tests before the swap.
2. **Phase B, on the integration branch `migration/scintilla`** from master after Phase A. SB0–SB6 merge into it in dependency order (§10), and the branch must be green after each merge.
   - **Master policy during Phase B (R1-14).** On master, the following are frozen until the cut-over: `[E]` feature work, and any change to `src/ui/editor*`, `src/core/document*` and `src/syntax/*`. The orchestrator refuses those merges, and a fix a user needs there is made on the integration branch instead. Editor-agnostic work (the gap analysis rows without `[E]`) may merge to master.
   - **Keeping the branch current.** At each SB merge, the orchestrator first merges master into `migration/scintilla`, then re-runs Release and ASan. A conflict in a file is resolved by that file's Phase B owner (§10.1), and the re-run output goes into that SB task's receipt.
3. **The cut-over** is one merge commit of `migration/scintilla` into master, after:
   - the Phase B output gates;
   - Abu Bakr's integration check;
   - the SB6 receipts and the rollback rehearsal.

   It ships as 1.1.0.

### 7.3 Rollback

- **Code:** revert the single cut-over merge commit. Phase A can stay: it is behaviour-preserving on `QPlainTextEdit`.
- **Installed app:** reinstall the 1.0.2 `.deb` (kept in `release/`, user-confirmed, sudo, as in Phase 1 §7). No data migration is needed, because no on-disk format changes.
- **Rehearsal (SB6, before the user installs):**
  1. In a scratch profile (scratch `HOME` and XDG dirs, never `~/.local/share/PrimeEdit` or `~/.config/PrimeEdit`), the new build leaves 3 drafts (Arabic, NBSP, CRLF), a `session.json` with folds and bookmarks, and settings.
  2. The 1.0.2 binary, extracted with `dpkg-deb -x` (never `dpkg -i`), runs against that profile, restores the drafts byte-identically, and reopens the session.
  3. The reverse direction is run the same way.
- **Pre-install procedure (user-confirmed):** Phase 1 §7's procedure — Save As on every untitled or modified tab, quit, copy `sessions/unsaved/` aside — is repeated.

## 8. Out of scope: follow-ups this migration makes cheap

Each one gets its own spec. From the gap analysis **[E]** rows:

| ID | Follow-up | Mechanism after S |
|---|---|---|
| F1 | Editing files over 50 MiB | needs a streaming draft encoder (the cache stores JSON text), buffer-based search, and a memory gate for edits |
| F2 | Clone to Other View | two Editors on one buffer; reopens D5 (§3.8.6) |
| F3 | Smart highlighting | indicator 13 |
| F4 | Style All Occurrences (5 styles), Jump up/down, copy styled text | indicators 14–18 |
| F5 | Change-history margin and navigation | `SCI_SETCHANGEHISTORY` |
| F6 | Hide Lines | `SCI_HIDELINES` |
| F7 | Fold level 1–8, fold current level; folding for Medium files | `SCI_FOLDCHILDREN`; a light lexer for Medium |
| F8 | Multi-select: undo latest added, skip current | `SCI_ROTATESELECTION`, `SCI_DROPSELECTIONN` |
| F9 | Begin/End Select (stream and column) | selection modes |
| F10 | Non-printing and control characters, Unicode EOL, wrap symbol, indent-aware wrap | `SCI_SETREPRESENTATION`, `SCI_SETWRAPVISUALFLAGS`, `SCI_SETWRAPINDENTMODE` |
| F11 | Vertical edge line(s) | `SCI_MULTIEDGEADDLINE` |
| F12 | Clickable URLs | indicator plus hotspot |
| F13 | HTML/XML tag matching, auto-close tags | `hypertext` styles |
| F14 | Call tips and API-file completion | `SCI_CALLTIPSHOW`, `SCI_AUTOCSHOW` |
| F15 | Export to HTML/RTF with colours | styles read back from the buffer |
| F16 | Paste binary (NUL-safe) | byte-level clipboard |
| F17 | Style Configurator | ThemeStyleMap per language |
| F18 | Plugin API v2 (buffer-level access) | `TextBuffer` |
| F19 | Long-line view "Edit anyway" (Q1) | full editable load with a warning |
| F20 | Remove `PieceTable`/`LargeFileReader` and their validation tests; remove the no-op `syncToDocument` | chore with Mohammed's consent to delete tests |
| F21 | Native Wayland (`qt6-wayland`) | needs IME on GNOME and KDE, and a single-instance raise test with an activation token (R1-13) |
| F22 | Enforce the long-line limit after load (paste, join, draft restore) | R5 (R1-16) |

## 9. Open questions

1. **Q1. Long-line files under 50 MiB (R3).** Recommendation: ship them read-only and wrapped. Add "Edit anyway" later (F19) only if it is missed. `L` is set by the R1-8 sweep, so R3 covers only lines the editor cannot lay out within the latency limits.
2. **Q2.** Resolved at the gate (R1-13): `qt6-wayland` is not added in this programme. Native Wayland is F21.
3. **Q3. Large-file editing deferred to F1.** Recommendation: accept. Keeping every editable document at 50 MiB or less keeps I5's draft cost and the save path exactly as Phase 1 measured them.
4. **Q4. Mouse bindings.** Keep today's (Ctrl+click goes to definition, Alt+click adds a caret, Alt+drag selects a rectangle), or switch to Notepad++'s (Ctrl+click adds a caret)? Recommendation: keep today's in this programme; rebinding belongs to the Shortcut Mapper.
5. **Q5. The 19 languages kept on JSON rules (§3.6).** Recommendation: accept the conservative split. Moving one later is a one-line catalog change plus a highlight case.

## 10. Breakdown

Every task: its own worktree from the stated base. Failing tests first, with the output recorded. No added or modified function over 50 lines. No new or rewritten file over 500 lines. No debug output. Conventional commits ending with the attribution trailer. Receipts paste real command output.

**Common verification (each task pastes it):**

```
cmake -S . -B <build> -DCMAKE_BUILD_TYPE=Release -DOPENGL_INCLUDE_DIR=/usr/include -DPRIMEEDIT_BUILD_TESTS=ON
cmake --build <build> -j$(nproc) && (cd <build> && ctest --output-on-failure)        # all pass
cmake -S . -B <asan> -DCMAKE_BUILD_TYPE=Debug -DOPENGL_INCLUDE_DIR=/usr/include -DPRIMEEDIT_BUILD_TESTS=ON -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
cmake --build <asan> -j$(nproc) && (cd <asan> && ctest --output-on-failure 2>&1 | tee asan.log)   # all pass
grep -c 'ERROR: AddressSanitizer\|ERROR: LeakSanitizer\|runtime error:' <asan>/asan.log        # 0
python3 tools/check_function_length.py --base <task base>
git diff -U0 <task base> -- tests/ | grep -E '^-.*\b(QCOMPARE|QVERIFY2?|QTRY_[A-Z_]+|QSKIP|QEXPECT_FAIL|QFAIL)\b'   # only the task's declared amendments
git diff -U0 <task base> -- tests/ | grep -E '^\+.*\b(QSKIP|QEXPECT_FAIL)\b'                                      # empty
for t in <each existing suite>; do diff <(<base build>/$t -functions) <(<build>/$t -functions); done              # no output
```

Never: `sudo`, `dpkg -i`, running `/usr/bin/prime-edit`, or touching `~/.local/share/PrimeEdit` or `~/.config/PrimeEdit`. Process tests use the build-tree binary with the suite's scratch `HOME` (as `tests/regression/CMakeLists.txt` sets it).

### 10.1 File ownership

A file not listed is frozen. A change it turns out to need goes back to the orchestrator.

| Files | Phase A | Phase B |
|---|---|---|
| `src/ui/editor.h`, `src/ui/editor*.cpp`, `src/ui/keychords.*`, `src/ui/editorids.h`, `src/ui/linenumberarea.*` | SA1 ali | SB2 ali |
| `src/ui/mainwindow*.cpp/.h` (including `mainwindow_npp_*.cpp`), `statusbar.*`, `functionlist.*`, `documentmap.*`, `markdownmode.*` | SA1 ali (facade calls only) | SB5 ali |
| `html-docs/guide.html` | frozen | SB5 ali only, all anchors including `#themes` (R1-6) |
| `RELEASE_NOTES.md` | frozen | SB6 abdullah |
| `src/syntax/lexercatalog.*` | frozen | SB2 ali creates it (lexer, `fold=1`, `commentSyntax`); SB3 ali extends it after SB2 merges (R1-5) |
| other `src/syntax/*`, `resources/syntax/*` (read-only) | frozen | SB3 ali |
| `src/core/session.cpp`, `src/ui/lspbridge.cpp` | SA2 uthman only (R1-6) | SB4 uthman (lspbridge); SB1 uthman (session.cpp, if needed) |
| `src/core/document.*` | SA2 uthman (`replaceContent` only) | SB1 uthman |
| `src/core/textbuffer.*`, `utf8scan.*` | — | SB1 uthman |
| `src/core/lspclient.*`, `lspchangetracker.*` | frozen | SB4 uthman |
| `tests/regression/editor_driver.h`, ported suites, `tests/validation/*`, `tests/feature/*` | SA3 abdullah | frozen, except A2 (SB5) and A3 (SB2) |
| `tests/regression/test_s*.cpp`, `tests/regression/CMakeLists.txt` | — | the owning task adds its own suite; abdullah owns the `CMakeLists` list and merges the entries |
| `third_party/*`, `cmake/Scintilla.cmake`, top-level `CMakeLists.txt`, `.github/workflows/build.yml`, `BUILD.md`, `tools/vendor_scintilla.sh` | — | SB0 abdullah (SB1–SB3 add their source files to `CMakeLists.txt` lists only) |

### 10.2 Phase A (master; behaviour-preserving)

**Phase A order (R1-6):** SA2 step 1 (`replaceContent`) → SA1 → SA2 step 2 (the facade callers in `session.cpp` and `lspbridge.cpp`) → SA3. There is no cycle: SA2 step 1 needs nothing from SA1.

**SA1 — Editor facade on QPlainTextEdit; UI callers.** Owner **ali**. Base: master after PR #6 (tree `ce5d206`). Depends on: SA2 step 1.

DoD:
- [ ] Every §4.2 facade member exists with its Phase A body.
- [ ] `syncToDocument` calls `Document::replaceContent` (from SA2). The dead viewport code (§2.3 item 1: `editor.cpp:112-132, 2892-2941` and `m_viewportStartLine`) is deleted, so nothing in `src/ui` calls `largeFileReader()`/`isLargeFile()` (grep shows none).
- [ ] `mainwindow*.cpp` (including the `mainwindow_npp_*.cpp` sites of §3.7.1 row 45), `statusbar` (now `selectionLength()`, R1-9), `functionlist`, `documentmap`, `markdownmode` and the plugin API call only the facade. `lspbridge.cpp` is SA2's, not SA1's (R1-6). `grep -nE 'textCursor\(|QTextCursor|QTextBlock|QPlainTextEdit::document|static_cast<QPlainTextEdit' src/ui --include=*.cpp | grep -v 'src/ui/editor\|src/ui/lspbridge'` is empty, except the terminal, commit dialog, eval panel, Markdown preview and the `mainwindow_npp_tools.cpp:147` result viewer, which are listed in the receipt.
- [ ] No behaviour change: all 24 suites green (Release and ASan) with zero test edits by SA1.

**SA2 — the bridge, then core callers to the facade.** Owner **uthman**. Step 1 depends on nothing. Step 2 depends on SA1 merged.

DoD:
- [ ] Step 1: `Document::replaceContent` added with the PieceTable body and no flag change. Covered by one case added to `test_t2_2_sync` (new case; no existing assertion changed).
- [ ] Step 2: `session.cpp:278-288,464-466` use `isEmpty()`, `cursorPosition()` and `firstVisibleLine()`. The draft keys and their meanings are unchanged.
- [ ] `lspbridge.cpp` uses only the facade.
- [ ] All 24 suites green.

**SA3 — driver and suite ports.** Owner **abdullah**. Depends on: SA1 and SA2 merged.

DoD:
- [ ] `editor_driver.h` per §4.3, with `keyChord` on `QTest::keyClick`.
- [ ] The 18 files in §5.3 use only the driver, the facade and `Document`.
- [ ] The assertion diff shows exactly A1.
- [ ] All 24 suites green in Release and ASan.
- [ ] The receipt lists, per file, the mechanical substitutions made.

Phase A output gate (Umar), then merge to master.

### 10.3 Phase B (`migration/scintilla`; one cut-over)

**SB0 — vendoring, build, CI, probe.** Owner **abdullah**. Depends on: Phase A merged.

DoD:
- [ ] `tools/vendor_scintilla.sh` run once from the pinned tarballs. `third_party/` tree, `SCINTILLA_PIN` and `MANIFEST.sha256` committed. The `vendor_manifest` CTest passes and fails on a planted one-byte edit (shown, then reverted).
- [ ] `cmake/Scintilla.cmake` builds `scintilla_qt` and `lexilla` and links them into `prime-edit-core`. A clean Release build shows no new warnings in `src/`.
- [ ] CI apt lines gain the Core5Compat dev package, with the name verified on `ubuntu-24.04` (receipt: `apt-cache policy`). Both CI jobs green on the branch's PR.
- [ ] `test_s0_probe` committed **first**, with both cases of §5.1 on a real `ScintillaEdit`: (a) `QTest::keyClick` and (b) `sendEvent`. The output is pasted. It decides `keyChord`'s body and whether `KeyChordDispatcher` is built (R1-12).
- [ ] `PRIMEEDIT_TIMING_ASSERTS` defined only for non-sanitizer Release builds (§5.2 rule 4, R1-11), shown with `cmake -LA` or a compile-definition check in both CI jobs.
- [ ] `BUILD.md` prerequisites and control snippet per §3.9. Licence files installed by an `install(FILES …)` rule.

**SB1 — TextBuffer, Document, load and save.** Owner **uthman**. Depends on: SB0.

DoD:
- [ ] Step 1: §4.1 declarations with placeholder bodies, so the build is green. `test_s1_buffer` and `test_s1_load` committed and failing (output recorded, including case 8a also run on the Phase A base).
- [ ] After SB1 merges into `migration/scintilla`, all 24 suites are still green with the old editor through the §3.2 bridge (pasted output).
- [ ] §3.2, §3.4 and §3.5 implemented. `writeAtomic` and `writeDirect` byte-identical to the base (`git diff` of those two functions is empty).
- [ ] B1 debug assertion; B2 view dropped before `finishSave(path, sha1, size)`, with the `changeCount` debug assertion (no read-only guard, R1-1); B3 `memchr` check.
- [ ] Every Phase-1 suite in §3.3 green unchanged. `test_s1_*` green.
- [ ] P1.11 and P1.12 strace receipts re-run on this branch.
- [ ] The receipt lists the `[UNVERIFIED]` items of §3.2 as resolved, with evidence.

**SB2 — Editor on ScintillaEdit.** Owner **ali**. Depends on: SB1 step 1 (header). It merges after SB1.

DoD:
- [ ] `test_s2_editor` committed failing first. Case 8b is also shown failing on the Phase A base.
- [ ] Every row of §3.7.1 assigned to SB2 implemented, including rows 39–44 (npp-qw1 members) and the minimal `LexerCatalog` with `commentSyntax` (R1-5). `test_t2_3_folding` and `test_npp_qw1` green in SB2.
- [ ] `KeyChordDispatcher` only if SB0's probe case (b) failed (R1-12). In that case its table is generated from vendored `KeyMap.cxx`, and a test asserts its size equals `MapDefault[]`'s entry count for the non-OS-X branch. Otherwise the receipt cites the probe output and the dispatcher does not exist (grep for `keychords` is empty).
- [ ] The Editor constructor attaches only and applies no `SyncTarget` (§3.3 D1); S4 case 9 passes once SB5 lands.
- [ ] The facade per §4.2 Phase B column.
- [ ] `linenumberarea.*` and the dead code (rows 34–36) deleted.
- [ ] The §3.7.2 `[UNVERIFIED]` rows (C3, 18, 38) resolved in the receipt.
- [ ] All suites green; A3 used only as declared.

**SB3 — highlighting and themes.** Owner **ali** (a separate spawn from SB2). Depends on: SB0, and SB2's `editorids.h`. It merges after SB2.

DoD:
- [ ] `test_s2_highlight` committed failing first.
- [ ] LexerCatalog covers all 96 languages exactly as §3.6. JsonRuleLexer and ThemeStyleMap implemented. `highlighter.*` deleted.
- [ ] The Arabic fallback screenshot is in the receipt (§3.6 `[UNVERIFIED]`).
- [ ] The `#themes` guide text is handed to SB5 in the receipt; SB3 does not edit `guide.html` (R1-6).

**SB4 — LSP incremental sync.** Owner **uthman**. Depends on: SB1 (the `changed` signal), SB2 (facade). It merges after SB2.

DoD:
- [ ] `test_s3_lsp` committed failing first.
- [ ] §3.8.1 implemented: tracker, `syncKind`, `didChangeParams`, flush of all trackers and before requests, full resync on `bufferReplaced`, no LSP for LargeFile.
- [ ] Manual receipt: `clangd` on a C++ file. Ten edits, including Arabic in a comment and an emoji, then a go-to-definition lands on the right line. The client log shows ranged `didChange` messages.

**SB5 — views, MainWindow, notices.** Owner **ali**. Depends on: SB2. It merges after SB3.

DoD:
- [ ] `test_s4_views` committed failing first.
- [ ] The §3.8.3 minimap, §3.8.5 Move to Other View, §3.8.8 consumers off, and view-kind notices (amendment A2) implemented.
- [ ] `html-docs/guide.html#largefile`, `#multicursor`, `#readonly`, `#shortcuts` and `#splitscreen` and `#themes` updated for §3.5, §3.6 and §3.7.2 (SB5 owns `guide.html`, R1-6).

**SB6 — integration verification and release candidate.** Owner **abdullah**. Depends on: SB0–SB5 merged.

DoD:
- [ ] §5.6 measurements against the §1 and §3.5.2 targets. Each number has its command and raw output. A miss is flagged, never rounded.
- [ ] The `L` sweep per §3.5.3 (64 KiB, 256 KiB, 512 KiB, 1 MiB; first paint, and keystroke-to-paint median and maximum of 20), and `L` set to the largest value that meets both limits, recorded in the receipt and `document.h` (R1-8).
- [ ] The R5 measurement: restoring a draft with a 5 MiB line (R1-16).
- [ ] Master merged into `migration/scintilla` and Release + ASan re-run, per §7.2 (R1-14).
- [ ] Full suite in Release and ASan on the integrated branch; 0 sanitizer reports.
- [ ] Rollback rehearsal per §7.3, in both directions.
- [ ] Candidate `prime-edit_1.1.0_amd64.deb` built in a scratch directory. `dpkg-deb -I` shows the §3.9 Depends and no `qt6-wayland`; `dpkg-deb -c` shows the licence files; `ldd` on the staged binary resolves everything.
- [ ] `RELEASE_NOTES.md` entry listing §3.7.2, and the plugin-visible changes to `EditorAPI::currentText`/`selectedText` (§3.8.7, R1-15).

After SB6: Umar output gates (SB2–SB5 per tier rules, plus the sampled parallel gate where its rule falls), Abu Bakr's integration check, then the single cut-over merge. Mohammed installs, following the §7.3 pre-install procedure.

### 10.4 Soft budgets

SA1 4 d · SA2 1 d · SA3 4 d · SB0 3 d · SB1 10 d · SB2 16 d · SB3 6 d · SB4 4 d · SB5 6 d · SB6 5 d. That is about 59 working days in total, inside the spike's 8–12 week estimate, with SB2/SB3 and SB1/SB0 overlapping. During this window the master policy in §7.2 applies (R1-14).

## 11. Round-1 findings index

Verdict: `docs/superpowers/reviews/2026-11-28-scintilla-migration-spec-verdict.md` (470444c), revise; 14 material, 5 minor, no blockers.

| ID | Sev | Resolution |
|---|---|---|
| R1-1 | material | §3.3 B2 and §3.4 step 5: `sha1`/`size` computed from the view, which is then dropped before any signal or `setSavePoint`; new private `finishSave(path, sha1, size)` (§4.1). The read-only guard is withdrawn; debug assertion on `TextBuffer::changeCount()`. S1 case 6 rewritten (a slot calls `setText` during the save). SB1 DoD updated |
| R1-2 | material | §3.3 B1 and §3.5.1 step 3.1: a failed strict validation falls through to the decode path (lossy, read-only), for lenient detection and for forced Reopen as UTF-8. §3.5.2: Preview cut on a code-point boundary (UTF-8, UTF-16 surrogates, UTF-32). S1 cases 15 and 16 |
| R1-3 | material | §3.3: rows added for D1 (the Phase B Editor constructor attaches only, with no `SyncTarget`), D4/D6 restore order, D8 (hash from the view; reload re-attach), D9 (zero-break default in `normaliseEols`), and P1.11 F2–F4 (F4 also routes LargeView/LongLineView drafts). S1 case 4 adds empty and no-break files; S1 case 17 (LongLineView draft → untitled); S4 case 9 (Move keeps undo to clean) |
| R1-4 | material | Header: base is master after PR #6 (tree = `ce5d206`), and Phase A is held until PR #6 merges. Every `editor.cpp`, `editor.h`, `mainwindow*.cpp` and `highlighter.cpp` citation re-cited against `ce5d206`. §3.7.1 rows 39–45 add `goToLineColumn`/`goToPosition`, `insertBlankLine`, `setBookmarks`, `selectWordIfNoSelection`, `selectBetweenMatchingBraces`, `commentSyntax` and the `mainwindow_npp_*` sites; row 12 adds `markAll(…, from, to)`. §4.2 regenerated. `commentSyntax` source after SB3: `LexerCatalog::commentSyntax`, owned by SB2 alone |
| R1-5 | material | Option (a): SB2 creates the minimal `LexerCatalog` (lexer, `fold=1`, `commentSyntax`), so `test_t2_3_folding` and `test_npp_qw1` pass in SB2; SB3 extends it (§3.1, §3.6, §3.7.1 rows 17 and 44, §10.1, SB2 DoD) |
| R1-6 | material | `lspbridge.cpp` is SA2's only (SA1 DoD grep excludes it). Phase A order: SA2 step 1 → SA1 → SA2 step 2 → SA3 (no cycle). `guide.html`, including `#themes`, belongs to SB5 alone; SB3 hands over the text (§10.1, §10.2, SB3/SB5 DoD) |
| R1-7 | material | §3.5.2 factors restated in KiB-correct form (peak 1.39 / 2.40, settled 1.30 / 2.30), with the whole-file array included (+1.0×) and legacy and UTF-16 factors derived. §1 targets are absolute, in MiB above the same-build baseline B. Gating rows G1–G4 plus the factor rows; a miss over 15 % needs a spec amendment before the merge (§1, §5.6, §7.2). SB6 adds 100 MiB windows-1256 and UTF-16LE |
| R1-8 | material | §3.5.3: the L sweep is 64 KiB, 256 KiB, 512 KiB and 1 MiB, measuring first paint and keystroke-to-paint (median and maximum of 20). L = the largest value with first paint ≤ 500 ms and a keystroke maximum ≤ 50 ms; if 64 KiB fails, a spec amendment decides. SB6 DoD |
| R1-9 | material | §4.2 and §4.1: the UTF-16 line-character index on every tier (cost measured in SB6); `selectionLength()` for the status bar (§3.8.8), so no `QString` is built; whole-file views are capped at UTF-16 length 2³¹ − 1, else Preview (§3.5.2, `setUtf16CapForTest`). S4 cases 7–8; S1 case 20 |
| R1-10 | material | §5.2 rule 2 and the §10 verification block: the grep covers `QCOMPARE`, `QVERIFY2?`, `QTRY_*`, `QSKIP`, `QEXPECT_FAIL` and `QFAIL`; no added `QSKIP`/`QEXPECT_FAIL`; each suite's `-functions` listing is diffed between the base build and the task build |
| R1-11 | material | §5.2 rule 4: `PRIMEEDIT_TIMING_ASSERTS` only in non-sanitizer Release builds; under ASan the timing cases print without asserting. SB0 DoD |
| R1-12 | material | §5.1 corrected from Umar's probe. `test_s0_probe` covers `keyClick` and `sendEvent` on a real `ScintillaEdit`. `KeyChordDispatcher` is built only if the `sendEvent` case fails (§3.1, §3.7.1 row 32, §4.3, SB0/SB2 DoD) |
| R1-13 | material | `qt6-wayland` is not added (§2.1, §3.9, `BUILD.md` snippet, §6, SB5/SB6 DoD). Q2 resolved. Native Wayland, with a raise test, is F21 |
| R1-14 | material | §7.2: during Phase B, master freezes `[E]` work and changes to `src/ui/editor*`, `src/core/document*` and `src/syntax/*`; editor-agnostic work may merge. At each SB merge, master is merged into `migration/scintilla` and Release + ASan are re-run; the file's Phase B owner resolves conflicts. §10.4, SB6 DoD |
| R1-15 | minor | Applied: §3.8.7 states the plugin-visible change to `currentText`/`selectedText`; C1 and C2 cross-reference it; SB6 lists it in `RELEASE_NOTES.md` |
| R1-16 | minor | Applied: §3.5.3 states that L is enforced at load only. SB6 measures restoring a draft with a 5 MiB line. Recorded as R5 (proposed known limit) and F22 |
| R1-17 | minor | Applied: (a) `replaceContent` restores the save point if it held before (§3.2, §4.1, S1 case 19); (b) the undo-to-clean change after `setText` is stated (§3.2, S1 case 18); (c) `Preserve` has lost its only production caller, and is kept for the contract and `test_t1_2_flags` (§3.2) |
| R1-18 | minor | Applied: B3 narrowed to exclude T1.8 read-only documents. §3.5.1 step 3 order is validate → BOM → NUL check → normalise; S1 case 4 |
| R1-19 | minor | Applied: (a) the `test_t1_1_save` citation corrected — it calls `setText` before `setReadOnly`; (b) nine test files plus `sync_failure_driver.cpp`; (c) C7 added (Medium lines from 8,000 chars to L become editable); (d) R2–R4 marked "proposed"; (e) the Notepad++ freeze claim dropped from Q1; (f) the `CLAUDE.md` performance targets added to §1 (G4, and the > 1 GB note) |

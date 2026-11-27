# Phase 1 — Data-loss and corruption fixes

**Status:** Approved (Umar round 2)
**spec_verdict:** docs/superpowers/specs/2026-11-26-phase1-data-loss-fixes.md — Umar round 2, verdict: approve · **Date:** 2026-11-26 · **tier: T3** (deletes data: stale cache directories, cache set replacement; replaces user files in place)
**fixes_artifact:** prime-edit 1.0.0 (`prime-edit_1.0.0_amd64.deb`, installed as `/usr/bin/prime-edit`). The installed binary matches the current source except for build metadata, so every defect below ships today.
**Base:** branch `fix/test-harness` at `0e79f0f` (Phase 0: suites build, run isolated, CI). It merges to `master` before any Phase 1 work merges. "Fails before the fix" means fails on `0e79f0f`.

Round-1 findings (Umar, verdict revise) are resolved inline and indexed in §10.

## 1. Objective

After this phase PrimeEdit must never silently lose or corrupt user text:

- **I1. Round trip.** Opening a file, typing a character, deleting it and saving through the real UI path (`MainWindow::saveFile`) produces byte-identical output. This holds for every supported encoding and for line endings that are uniform within the file. Mixed line endings are the one documented exception (D11).
- **I2. Modified tracking.** Every user edit, and every pending encoding or line-ending conversion, marks the document modified. Undo cannot make a document look clean unless it is truly back to its saved state. Save, the close prompt and the unsaved cache all read the same flag.
- **I3. No lossy write without consent.** A save that would drop or replace characters is refused, or needs a confirmation that names the count. Those saves are: characters unmappable in the target encoding, a file opened from lossily decoded bytes, or a partial view of the file.
- **I4. Durable writes.** File saves and every persistence file are written atomically: the unsaved cache, `session.json`, named sessions in `sessions/*.json`, `settings.json` and `macros.json`. A failed write reports failure and leaves the previous content intact.
- **I5. Unsaved work survives.** Untitled and unsaved documents survive quit, a launch with file arguments, a second launch while one instance runs, a draft whose original file can no longer be opened, and a corrupt cache entry. A crash loses at most the edits since the last periodic cache write, which happens every 30 s.

## 2. Scope

| ID | Defect (review evidence) |
|---|---|
| D1 | Edits after the 2 s RTL pass are never flagged modified (`editor.cpp:267-313`, hook `:177-182`). The same mechanism affects a restored draft after type plus undo (`session.cpp:319-322`), and a pending conversion after edit plus undo (`editor.cpp:177`). |
| D2 | Code folding deletes the folded text; Save writes ` [...]` (`editor.cpp:1858-1913`). |
| D3 | Partial views (files over 50 MB, and minified files over 20 MB) are editable; Save, or a conversion followed by Save, truncates the file (`editor.cpp:102-131`, `document.cpp:40-42,167,240`). |
| D4 | Unsaved cache destroyed in several ways (`session.cpp:220-326`, `main.cpp:82-98`): <ul><li>wiped 30 s after a launch with file arguments;</li><li>clobbered by a second instance;</li><li>cache files deleted on restore;</li><li>a torn `session.json` makes restore skip the cache;</li><li>a non-atomic directory swap;</li><li>a draft restored into the wrong editor when its original cannot be opened (`session.cpp:309`);</li><li>an unparsable entry dropped (`:302`);</li><li>untitled editors with content not cached because the content check reads the empty PieceTable (`:236`). This last one is the failing `crash_test` case.</li></ul> |
| D5 | Split view: <ul><li>pane-2 documents never cached on quit, and settings handlers skip pane 2 (`session.cpp:227`, `mainwindow.cpp:226-278`);</li><li>closing a tab can free a `Document` that the other pane still shows (`mainwindow.cpp:975-980`);</li><li>Clone creates an independent copy whose save overwrites the other view (`mainwindow.cpp:2313-2318`, `mainwindow_extended.cpp:609`);</li><li>tab-bar-1 close requests act on the focused pane (`mainwindow.cpp:1128`).</li></ul> |
| D6 | Encoding corruption (`document.cpp:88-193`, `charsetdetector.cpp:87`): <ul><li>legacy encodings detected but decoded as UTF-8;</li><li>Medium mode ignores the detected encoding;</li><li>detection scans only 8 KB;</li><li>UTF-16 BOM dropped;</li><li>the "UCS-2 BOM" items write no BOM;</li><li>Convert to ANSI silently writes `?`;</li><li>the cached encoding is ignored on restore (`session.cpp:299-300`).</li></ul> |
| D7 | Save is non-atomic and reports success after ENOSPC or another I/O error (`document.cpp:177`). The modified flag is cleared before the save is attempted (`editor.cpp:1295`). |
| D8 | Every own save raises "modified by another program. Reload?"; accepting reloads only the visible editor and discards unsaved edits (`documentmanager.cpp:181`, `mainwindow.cpp:186-210`). |
| D9 | A file with no line break, including every empty file, is detected as CRLF (`document.cpp:394`). |
| D10 | Persistence files are written by truncate-then-write; a torn read resets settings to defaults, which are then saved over the good file (`settings.cpp:32,126`, `session.cpp:29,71`, `macrorecorder.cpp:23`). |
| D11 | **New in round 1:** the Editor's save path uses `toPlainText()`, which turns U+00A0 into U+0020 and U+2028/U+2029 into LF. One keystroke plus Save rewrites every no-break space in the file. Load also normalises mixed line endings to the majority (`document.cpp:394`, `editor.cpp:1294`). |
| D12 | "Read-only" is advisory only. Lossy decode and Medium-mode long lines call only `Document::setReadOnly`, the Editor never mirrors it, and both close prompts skip read-only documents, so edits are discarded without asking (`document.cpp:104-106`, `mainwindow.cpp:954,2418`). |

**Out of scope, with reasons:**
- **Remote-file mapping lost after restore** (`mainwindow_extended.cpp:1179`). It needs the remote-editing redesign; tracked for Phase 2. For now, D4 restores such drafts as untitled tabs that keep `originalFilePath`, so nothing is written to the stale `/tmp` copy.
- **The "Auto-save" crash-recovery preference** writes stale text and is never read back (`documentmanager.cpp:190`). It is superseded by the D4 cache. Phase 1 hides the preference; removing the dead code is Phase 3.
- **A true shared-document Clone.** Clone is removed in this phase (D5). A shared-buffer Clone needs the per-document holder architecture and is a Phase 3 feature.
- Editing commands that corrupt text but can be undone, security, crashes outside these paths, search options, performance, the memory-mapped viewer, LSP, and the unreachable UI: Phases 2 and 3.

## 3. Design

### D1 — modified tracking (I2)
- `Editor::runSyncedEdit(SyncTarget target, std::function<void()> edit)` sets `m_syncing`, runs `edit`, then sets the flags to `target` in **both** `QTextDocument` and `Document` while `m_syncing` is still set.
- Target per site:
  - RTL pass: `Preserve`, which restores the prior state of both flags.
  - Load and reload: `Clean`.
  - Draft restore: `Modified`. `QTextDocument::setModified(true)` clears the clean index, so undo cannot make the draft look clean.
- `Document` gains a format-dirty flag. `isModified()` returns `contentModified || formatDirty`. `setEncoding` and `setLineEnding` set it, and only a successful save or load clears it. Text undo therefore cannot hide a pending conversion.
- RTL pass: change a block's format only when the block contains Arabic, or when it was previously set to RTL. Blocks in `LayoutDirectionAuto` are left alone.
- Known limitation, recorded and not fixed: RTL format changes on Arabic text create undo entries.

### D2 — folding is view-only
- Fold by `QTextBlock::setVisible(false)` on the body blocks, then `markContentsDirty` and a viewport update. Text is never removed or inserted. `m_foldedRegions`' cached text is deleted.
- Fold state is derived, not stored. A header block carries a `FoldHeader` marker in `QTextBlockUserData`. A region is folded when its header is marked and the next block is hidden. No `QTextBlock` handles are kept.
- **Unfold rules:**
  - Any cursor or selection that enters a hidden block unfolds its region first. This is enforced in `cursorPositionChanged`, and in Find, Go to Line and multi-cursor before they edit.
  - Removing a header unhides its body.
  - Undo that re-creates hidden text leaves it visible.
- **Session contract:** `Editor::foldedRegions()` changes from `QMap<int, QStringList>` (`editor.h:137`) to `QList<int>`: header block numbers, computed on demand. Its one consumer, `session.cpp:148` (`.keys()`), changes in W0 (§9).

### D3 — partial views are not saveable (I3)
- `Document::isPartialView()` is true in `LargeFile` mode, which covers files over 50 MB and minified files over 20 MB.
- A partial view is read-only (D12 semantics):
  - `save`/`saveAs` return `SaveResult::PartialView`.
  - Save, Save As, Save All and the encoding and line-ending conversions are disabled for that tab.
  - `setEncoding`/`setLineEnding` refuse and leave the flags untouched.
- The "showing first N of M" banner moves to the notification bar. No text is appended to the buffer.

### D4 — unsaved cache (I4, I5)
**Single instance**
- **Startup order:**
  1. Take `QLockFile` at `AppDataLocation/instance.lock`, with the default 30 s stale time. `Settings::load()` moves after this step (`main.cpp:65`), because D10's corrupt-file copy is a write.
  2. If that fails, forward to the running instance (below).
  3. Otherwise `QLocalServer::removeServer(name)`, then `listen(name)`.
  4. Run cache recovery.
  5. Create the window.
- **Server name:** the absolute path `$XDG_RUNTIME_DIR/prime-edit-<hash>.sock`, where `<hash>` is the first 16 hex digits of SHA-1 of `AppDataLocation`. If `XDG_RUNTIME_DIR` is unset, use `AppDataLocation/instance.sock`. The name is therefore per user and per profile: a test with a scratch HOME can never reach the live editor, and another user cannot take the name first.
- **Forwarding protocol:** one JSON message, `{"v":1,"files":[absolute paths],"goto":<int or null>}`, newline-terminated. The holder replies `{"ack":true}` after queueing the opens and raising its window. Paths are made absolute against the forwarding process's working directory before sending.
- **Timeouts:** connect 2 s, ack 5 s. On timeout, the second launch shows an error ("PrimeEdit is running but not responding") and exits 1. It never starts a second writer.
- `--new-window` is forwarded the same way (Q2 below).
- The option is `-g/--goto` (`main.cpp:57`).

**Restore**
- Drafts are restored on every launch, independent of file arguments and of the "Restore previous session" preference. That preference now controls only reopening saved files. A corrupt `session.json` does not affect draft restore. File arguments open after the drafts.
- `MainWindow::openFile` returns `Editor*` (null on failure).
- Cached text is applied only to the editor returned for `originalFilePath`, after checking that its document path matches.
- If the open fails, the text goes into a new untitled tab that keeps `originalFilePath` as the Save As default, and a notification names the file.
- The cached encoding and line ending are applied (D6). Line ending is stored under a new optional `"lineEnding"` key; entries without it (v1.0.0) use the detected or default ending. The target state is `Modified` (D1).
- An entry that fails to parse is moved to `sessions/unsaved.corrupt/` and kept.

**Writing the set**
- The writer collects every document from `MainWindow::allEditors()` (D5), with `hasContent` taken from the editor's `QTextDocument` (`!isEmpty()`), not the PieceTable. This is the fix for the failing `crash_test` case.
- Restore registers every draft it places. A draft that restore could not place in an editor is still written. The writer never drops an entry restore did not place.
- **Atomic replacement:**
  1. Write the set into `unsaved.new/`: each file written with `QSaveFile`, then `fsync` the directory.
  2. Rename `unsaved/` to `unsaved.old/`.
  3. Rename `unsaved.new/` to `unsaved/`.
  4. `fsync` `sessions/`.
  5. Delete `unsaved.old/`.

  Fixed names are safe because the single-instance lock is held. A pid suffix is not needed (R1-15).
- **Recovery table**, run under the lock before the window exists:

| On disk | Action |
|---|---|
| `unsaved/` only | Normal. |
| `unsaved.new/` (any) | Incomplete write: delete it. |
| `unsaved/` + `unsaved.old/` | Step 5 did not finish: delete `unsaved.old/`. |
| `unsaved.old/` without `unsaved/` | Crash between steps 2 and 3: rename `unsaved.old/` to `unsaved/`. |
| `unsaved_tmp/` (v1.0.0 leftover) without `unsaved/` | Adopt it: rename it to `unsaved/`. |
| `unsaved_tmp/` + `unsaved/` | Merge: copy entries whose UUID is not already in `unsaved/`, then delete `unsaved_tmp/`. |

- The v1.0.0 30 s wipe is removed. The periodic writer runs every 30 s and only ever replaces the set with the current one, including unplaced drafts.

### D5 — split view
- **Clone removed.** Delete the View > "Clone to Other View" action and `cloneToOtherView()`. `splitView()` opens an empty second pane instead of cloning. Move to Other View remains. With no Clone, a `Document` is shown by at most one editor.
- **Lifetime guard:** before `closeDocument`, `closeFile` scans `allEditors()` for another editor of the same `Document`, reusing the logic at `mainwindow.cpp:2407-2414` generalised to both panes. This is defence in depth now that Clone is gone. No reference-counting API is added (R1-15).
- `MainWindow::allEditors()` returns the editors of both panes. `Session::save`, `saveUnsavedDocuments` and the settings handlers (`mainwindow.cpp:226-278`) use it.
- Close requests are routed by the sending tab widget, not by focus.

### D6 — encodings (I1, I3)
- **Codec.** `TextCodec` (`src/core/textcodec.{h,cpp}`) wraps ICU `ucnv`; ICU becomes required. UTF-8, UTF-16 and UTF-32 use `QStringConverter`, with `hasError()` checked.
  - `decode(bytes, name) -> {QString text, bool lossless}`.
  - `encode(text, name) -> {QByteArray bytes, QList<int> unmappable}`. Positions are `QString` UTF-16 indices.
- **Lossless** means decoding raised no error **and** `encode(decode(bytes)) == bytes`, for every codec including the UTF family. This catches many-to-one maps (CP932 duplicates, 0x5C yen/backslash, Big5) and lone surrogates.
- **Detection** runs on the whole file in Small and Medium modes, with the 8 KB probe kept for partial views only. It returns `{name, hasBom}`. BOM presence is preserved on save.
- **Load:** if the decode is not lossless, the document opens read-only (D12) with a notification offering "Reopen with Encoding…".
- **Reopen with Encoding…** (new, Encoding menu) re-decodes the original bytes with the chosen codec. It reloads; it does not convert. It is disabled when the document has unsaved edits, unless the user confirms discarding them.
- **Convert (`Encode in …`):** runs `encode` first. If anything is unmappable, show the count and the first 5 positions as line:column, and offer Cancel or "Replace N characters". Replace is a single undoable edit in the editor that replaces each unmappable character with `?`, so the user sees exactly what will be written; no Document flag is involved.
- **Save-time EncodeLossy:** when typed text cannot be encoded, `save` returns `SaveResult::EncodeLossy` and the file is untouched. The dialog offers Save As UTF-8, "Replace N characters", or Cancel.
- **UCS-2 BOM items** map to UTF-16LE/BE with BOM.
- Restore applies the cached encoding.

### D7 — atomic, checked save (I4)
- `Document::saveAs` uses `QSaveFile` and `commit()`. Verified in round 1: a write through a symlink updates the target and keeps the link, and the file mode is preserved.
- It returns `SaveResult {Ok, PartialView, EncodeLossy, ReadOnly, IoError}`, with `Document::lastError()` for the message.
- `syncToDocument` no longer clears `QTextDocument` modified. Both flags are cleared only after `SaveResult::Ok`. On failure the document stays modified, through type plus undo too (D1 format and clean index).
- **When an atomic save is unsafe**, write directly and warn:
  - **Directory not writable** (QSaveFile fails, plain write would succeed): return `IoError`. MainWindow offers "Save directly (not atomic)" **only when `Document::directWriteAvailable()` is true**, meaning the failure was the directory, never ENOSPC or another error that a direct write would turn into truncation. On confirm, retry with `QSaveFile::setDirectWriteFallback(true)`.
  - **Hard link** (`st_nlink > 1`) or **owner differs from euid:** atomic replace would break the link or change ownership. Write directly with `QFile` (truncate and write, then flush and `fsync`) and show a one-line notice. `setDirectWriteFallback(true)` is used only for the 0555-directory confirm path, because it writes directly only when it cannot create the temp file. No prompt, because the user's intent to save is clear.
  - **Target not writable** (e.g. mode 0444): `IoError` "File is read-only", with no fallback.

### D8 — external-modification detection
- After its own save, `DocumentManager` records the file's `(mtime, size, sha1 of bytes written)`. A watcher event whose current file matches the record is ignored.
- Reload applies to the `Document` and its single editor (no Clone), through `runSyncedEdit(Clean)`.
- With unsaved edits, the bar offers "Reload (discard my changes)" and "Keep mine". It never reloads silently, and background tabs get the same bar when activated.

### D9 — line-ending detection
A file with no line break, including an empty file, uses the line-ending default from settings (LF today), not CRLF.

### D10 — persistence files (I4)
- `settings.json`, `session.json`, named sessions (`sessions/*.json`) and `macros.json` are written with `QSaveFile`.
- **Unparsable file on load:** copy it to `<name>.corrupt-<yyyyMMdd-hhmmss>` and leave the original in place. Defaults are used in memory, and nothing is saved until the user changes a setting. Only then is the file written, from the in-memory state.
- Multi-instance last-writer-wins goes away with the D4 lock.

### D11 — text fidelity on the save path
- `Editor::rawText()` (`QTextDocument::toRawText()`, with U+2029 block separators mapped to LF) is the only accessor for document text on the save path **and** in the cache writer (`session.cpp:246`). U+00A0 and U+2028 are preserved in both.
- **Mixed line endings:** load records whether the file's line endings were mixed. On save, a mixed file is normalised to its majority ending, and the notification bar says so once per document ("Mixed line endings will be saved as CRLF"). This is the documented exception to I1. Preserving per-line endings is Phase 3.

### D12 — read-only semantics
- The Editor calls `setReadOnly(document->isReadOnly())` at bind time and on `readOnlyChanged`, for every cause: partial view, lossy decode, Medium-mode long lines, and a file without write permission.
- Both close prompts key on `isModified()` alone. `save` returns `SaveResult::ReadOnly` for read-only documents.

## 4. Contracts

- `enum class SaveResult { Ok, PartialView, EncodeLossy, ReadOnly, IoError };` and `QString Document::lastError() const;`
- `bool Document::isPartialView() const;` `bool Document::isFormatDirty() const;` signal `Document::readOnlyChanged(bool)`.
- `enum class SyncTarget { Preserve, Clean, Modified };` and `void Editor::runSyncedEdit(SyncTarget, const std::function<void()>&);`
- `struct DecodeResult { QString text; bool lossless; };` `struct EncodeResult { QByteArray bytes; QList<int> unmappable; };` and `TextCodec::decode/encode`.
- `QList<Editor*> MainWindow::allEditors() const;` `Editor* MainWindow::openFile(const QString&);` `QString Editor::rawText() const;` `QList<int> Editor::foldedRegions() const;`
- `bool Document::directWriteAvailable() const;` `bool Document::lastSaveWasDirect() const;` `bool Document::wasDecodedLossily() const;` `bool Document::hadMixedLineEndings() const;` `bool Document::reloadWithEncoding(const QString&);` `static void Document::setSizeThresholdsForTest(qint64 medium, qint64 large);`, plus `MainWindow::showNotification(const QString&)`. §9 gives the full signatures.
- Instance IPC: newline-terminated JSON `{"v":1,"files":[...],"goto":N|null}` and the reply `{"ack":true}`.
- Cache entries: the same JSON per document plus one optional `"lineEnding"` key. v1.0.0 caches restore, and v1.0.0 ignores the new key. Directories: `sessions/unsaved/`, transient `unsaved.new/` and `unsaved.old/`, and the kept `unsaved.corrupt/`.

## 5. Test strategy

Every defect, and every sub-defect listed in §2, gets a regression test that fails on the W0 base and passes with the fix. The new suites need W0's declarations to compile. W0 is required not to change behaviour, so the W0 base is behaviourally `0e79f0f`. The test and the fix land together. The new QtTest suite `tests/regression/` links `prime-edit-core`, is registered with CTest, and calls `isolateFromUserData()`.

- **I1 round-trip matrix, through the real path.**
  - Steps: open in a MainWindow, type a character, delete it, `MainWindow::saveFile`, assert the bytes are identical.
  - Fixtures, all generated in the test: UTF-8 with and without BOM; UTF-16LE/BE with and without BOM; UTF-32LE; Windows-1252; Shift-JIS; ISO-8859-6; Windows-1256. Each with LF, CRLF and CR. Plus empty, no-newline, U+00A0, U+2028, a mixed CRLF/LF file (expect the documented normalisation) and a lone CR.
  - Each legacy fixture runs twice: once by detection (asserting the detected name where detection is in scope) and once by Reopen with Encoding.
  - Medium mode is forced through a test hook on the threshold.
- **D1:**
  - RTL pass, then type: modified, Save enabled, draft cached. Repeat with Arabic content.
  - A restored draft stays modified after type plus undo.
  - A conversion stays modified after type plus undo.
- **D2:**
  - Fold, type, save: the full text is on disk.
  - Fold, then Find a match inside the fold: the region unfolds before the replace.
  - Delete a header: its body is visible.
  - Session round trip of fold state.
- **D3:**
  - A 51 MB generated file: an edit attempt is rejected; Ctrl+S, then convert and Save, leave the bytes unchanged with `PartialView`.
  - A 21 MB minified fixture behaves the same.
- **D4:** tests run the build-tree binary only, with HOME, XDG_DATA_HOME, XDG_CONFIG_HOME and XDG_RUNTIME_DIR all in scratch.
  - A second launch with a file argument forwards it (with an absolute path) and exits 0, and the first instance's drafts remain.
  - A holder that never acks makes the second launch exit 1.
  - Each recovery-table state is built directly on disk; startup recovery then yields the expected `unsaved/`.
  - A launch with file arguments restores drafts.
  - A corrupt `session.json` still restores drafts.
  - An unreadable original: the draft lands in a new tab, and the previously active tab is unchanged.
  - A corrupt entry survives the next write in `unsaved.corrupt/`.
  - The `crash_test` theme-switch case passes.
- **D5:**
  - Move a modified untitled doc to pane 2, then quit: it is cached.
  - The settings change applies to the pane-2 editor.
  - A close from tab bar 1 with pane 2 focused closes the pane-1 tab.
  - Clone is absent.
  - All under ASan.
- **D6:** one named test per sub-defect:
  - a non-UTF-8 byte after 8 KB;
  - Medium-mode UTF-16;
  - UCS-2 BOM items write a BOM;
  - Convert to ANSI with Arabic shows the unmappable prompt and writes nothing on Cancel;
  - a lossy decode opens read-only;
  - Reopen with Encoding recovers Windows-1252;
  - the BOM is kept on save;
  - the cached encoding is applied on restore;
  - CP932 many-to-one bytes open read-only.
- **D7:**
  - **ENOSPC equivalent:** a child process with `setrlimit(RLIMIT_FSIZE)` and SIGXFSZ ignored. Assert `IoError`, the original bytes intact, and still modified after type plus undo.
  - Mode 0444 gives `IoError`.
  - A writable file in a 0555 directory gives `IoError` plus the direct-write path on confirm.
  - A hard-linked file keeps the link (direct write).
  - A symlink keeps the link, and permissions are kept.
- **D8:**
  - Own save shows no bar.
  - External change plus unsaved edits gives the bar, and no silent reload.
  - Reload of a background tab updates it on activation.
- **D9:** open an empty file, type `a`, Enter, `b`, save. The bytes are `61 0A 62`.
- **D10:** a truncated `settings.json`, `session.json`, named session and `macros.json` each yield a `.corrupt-*` copy, the original unchanged, and no write until a change is made.
- **D11:** NBSP and U+2028 survive type, delete, save. Mixed line endings normalise with the notice.
- **D12:** a lossy-decoded document refuses keystrokes. A modified document that later became read-only prompts on close.
- **CI:** add an ASan and UBSan job (`-fsanitize=address,undefined`) that runs all suites.

## 6. Blast radius

Save path, load path, the encoding menu, session and cache persistence, startup (single instance), split view and folding: the core of the editor. Existing caches, `session.json` and `settings.json` stay readable.

Behaviour changes users will notice:
- (a) A second launch opens its files in the running window.
- (b) Files that cannot be decoded losslessly open read-only, with Reopen with Encoding.
- (c) Partial views (files over 50 MB, and minified files over 20 MB) are read-only.
- (d) Lossy conversions and lossy saves ask first.
- (e) Clone to Other View is removed; Move remains.
- (f) Mixed line endings are normalised, with a notice.

## 7. Rollback and install

- **Rollback** is the 1.0.0 package: keep `release/prime-edit_1.0.0_amd64.deb` and reinstall it with `sudo dpkg -i`. **Rollback procedure** (rehearsed 2026-11-27):
  1. Quit 1.0.1.
  2. Copy `~/.local/share/PrimeEdit/PrimeEdit/sessions/unsaved/` aside.
  3. Install the 1.0.0 package from `~/primeedit-rollback/`.
  4. Start 1.0.0 **with no file arguments**, and only while `session.json` exists.

  1.0.0 restores drafts only from `Session::restore()`, which returns early when `session.json` is missing or when files are given on the command line. 1.0.0's 30 s writer then deletes the cache. The rehearsal confirmed both outcomes: drafts are lost without `session.json` and restored identically with it. Per-commit reverts are not independent: D3 uses D7's SaveResult, D10 relies on D4's lock, and D4 relies on D5's `allEditors()`.
- **Rehearsal** (integration step, before the user installs):
  1. Run the new build in a scratch profile and leave drafts and a leftover `unsaved.old/`.
  2. Install 1.0.0 into a scratch prefix.
  3. Confirm 1.0.0 restores those drafts. A leftover `unsaved.old/` is ignored by 1.0.0, which is harmless.
- **Pre-install procedure, which the user confirms** (R1-12):
  1. In each running instance, use Save As on every untitled or modified tab.
  2. Quit the instances one at a time. After each quit, copy `~/.local/share/PrimeEdit/PrimeEdit/sessions/unsaved/` aside.
  3. Install the new `.deb`.
  4. Launch and confirm every draft is present before deleting the copies.

  A backup of the current cache already exists (`~/primeedit-drafts-backup-20261126-225010/`, 12 drafts).

## 8. Open questions

- **Q2.** Recommendation: forward `--new-window` like any other launch until real multi-window support exists within one process.
- **Q3.** Recommendation: ICU is required, because a silent fallback is how the corruption happened.
- **Q1** is resolved: Clone is removed (§3 D5).

## 9. Architecture and task breakdown

Phase 1 branches from `feature/markdown-preview` at `9f5a683`, which is `0e79f0f` plus the Markdown preview feature. That feature uses only the public `Editor` API (`document()`, `toPlainText()`, properties). Line numbers in §2 and §3 refer to `0e79f0f`. In `mainwindow.cpp` they are shifted by up to 16 lines at `9f5a683`, so tasks below name functions, not lines.

### 9.1 Architecture

#### Components after the change

| Component | Files | Responsibility after Phase 1 | D |
|---|---|---|---|
| Document | `src/core/document.{h,cpp}` | Owns a file's bytes-to-text contract. Load picks the mode, detects the charset on the whole file (8 KB probe for partial views only), decodes through TextCodec, records `lossless`, BOM and mixed line endings, and opens read-only if the decode is lossy. Save refuses `ReadOnly`/`PartialView`, encodes through TextCodec (`EncodeLossy` writes nothing), writes through `QSaveFile` or a deliberate direct write, and clears `contentModified` and `formatDirty` only on `Ok`. After every successful write it emits `saved(path, sha1, size)`. | D1 D3 D6 D7 D9 D11 D12 |
| TextCodec (new) | `src/core/textcodec.{h,cpp}` | The only byte/text converter in the save and load paths. ICU `ucnv` for legacy codecs, `QStringConverter` with `hasError()` for UTF-8/16/32. Adds or strips a BOM when the name carries `-BOM`. `lossless` = no decode error and `encode(decode(b)) == b`. Stateless functions, no QObject. | D6 |
| CharsetDetector | `src/core/charsetdetector.{h,cpp}` | `detect(bytes)` returns `{name, hasBom}` over the bytes it is given. Document passes the whole file in Small and Medium mode, and 8 KB for partial views. | D6 |
| Session | `src/core/session.{h,cpp}` | Three jobs. **Recovery:** `recoverCacheDirectories()` applies the §3 D4 recovery table, under the lock, before any window exists. **Restore:** `restoreDrafts(MainWindow*)` runs on every launch; `restoreFiles(MainWindow*)` runs only under the preference. **Cache writer:** `saveUnsavedDocuments(MainWindow*)` collects `allEditors()`, keeps unplaced drafts, and replaces `unsaved/` atomically. `session.json` and named sessions are written with `QSaveFile`, and a corrupt file is copied aside, never overwritten. | D4 D5 D6 D10 |
| SingleInstance (new) | `src/core/singleinstance.{h,cpp}` | In `prime-edit-core` (not `main.cpp`) so tests link it. Holds the `QLockFile` for the process lifetime, owns the `QLocalServer`, speaks the §4 IPC protocol, and emits `openRequested(QStringList files, int gotoLine)`. As a client, `forward()` blocks for at most 2 s to connect and 5 s for the ack, before any event loop runs. Qt Network is already linked. | D4 |
| main | `src/main.cpp` | Startup ordering only (below). | D4 D10 |
| DocumentManager | `src/core/documentmanager.{h,cpp}` | Records `(mtime, size, sha1)` from `Document::saved`. Suppresses a watcher event whose current file matches the record, so `fileExternallyModified` means a real external change. | D8 |
| Settings | `src/utils/settings.{h,cpp}` | `QSaveFile` writes. A corrupt file on load is copied to `.corrupt-<ts>` and the defaults are held in memory; `save()` is a no-op until a setter changes a value. `defaultLineEnding()` (already `"LF"`) is the D9 source. | D9 D10 |
| FileUtils | `src/utils/fileutils.{h,cpp}` | Adds `bool FileUtils::fsyncDirectory(const QString&)` (POSIX `open(O_RDONLY|O_DIRECTORY)` plus `fsync`), used by the cache writer. | D4 |
| MacroRecorder | `src/core/macrorecorder.cpp` | `macros.json` through `QSaveFile`, with the same corrupt-copy rule as Settings. | D10 |
| Editor | `src/ui/editor.{h,cpp}` | Keeps the two modified flags in step through `runSyncedEdit`. `rawText()` is the only text accessor for save and cache. `syncToDocument` no longer touches the `QTextDocument` flag. The RTL pass uses `Preserve` and touches only Arabic or previously-RTL blocks. Folding is view-only (`setVisible`, derived state). Mirrors `Document::isReadOnly()` at bind time and on `readOnlyChanged`. A partial view gets no banner text in the buffer. | D1 D2 D3 D7 D11 D12 |
| MainWindow | `src/ui/mainwindow.{h,cpp}`, `src/ui/mainwindow_extended.cpp` | `allEditors()` (both panes) and `Editor* openFile()`. The save UI maps each `SaveResult` to a dialog or notice and clears the flags only on `Ok`. Encoding menu: the convert preflight, the UCS-2 BOM mapping, and Reopen with Encoding…. The reload bar with "Reload (discard my changes)" and "Keep mine", which also appears on background tabs when activated. Close prompts key on `isModified()`. Clone is removed and close requests are routed by the sender. Per tab, Save, Save As, Save All and conversions are disabled for partial views. | D3 D5 D6 D7 D8 D12 |
| NotificationBar, StatusBar, PreferencesDialog | `src/ui/notificationbar.{h,cpp}`, `src/ui/statusbar.{h,cpp}`, `src/ui/preferencesdialog.{h,cpp}` | Info messages and action-button messages. The status-bar encoding menu routes through the same MainWindow slots, so it gets the same preflight. The Auto-save preference is hidden (§2, out of scope). | D3 D6 D8 |

#### Contracts named here (in addition to §4)

Each identifier below crosses a workstream boundary, so it is fixed here before dispatch. W0 lands every declaration. A row marked *W0-implemented* has its real, final body in W0. The other rows are declared in W0 and defined by the named task. Nothing calls them before that task merges into the caller's branch, which the dependencies guarantee.

| Contract | Producer → consumer | Body |
|---|---|---|
| `enum class WriteMode { Atomic, AllowDirect };` `SaveResult Document::save(WriteMode = WriteMode::Atomic);` `SaveResult Document::saveAs(const QString&, WriteMode = WriteMode::Atomic);` | W1 → W3 | W0-implemented as a behaviour-preserving map of today's code (`Ok`/`IoError`). T1.1 gives the real semantics. |
| `bool Document::directWriteAvailable() const;` — true after `IoError` only when the atomic temp file could not be created and the target itself is writable | W1 → W3 | T1.1. MainWindow offers "Save directly (not atomic)" only when this is true. Offering it after ENOSPC would truncate the original. |
| `bool Document::lastSaveWasDirect() const;` — the hard-link or foreign-owner case | W1 → W3 | T1.1 (drives the one-line notice) |
| `bool Document::wasDecodedLossily() const;` `bool Document::hadMixedLineEndings() const;` `bool Document::reloadWithEncoding(const QString &name);` | W1 → W3 | T1.3, T1.2, T1.3 |
| `static void Document::setSizeThresholdsForTest(qint64 medium, qint64 large);` | W1 → tests (T1.3, T4.1) | T1.3 |
| `QString Editor::rawText() const;` — `toRawText()` with U+2029 mapped to LF, U+00A0 and U+2028 kept | W2 and W1 (cache writer) | W0-implemented, unused until T1.5 and T2.2 adopt it |
| `void Editor::runSyncedEdit(SyncTarget, const std::function<void()>&);` (§4) | W2 → W1 (restore), W3 (reload, post-save) | W0-implemented. It is the D1 contract and nothing in it depends on later tasks. |
| `QList<int> Editor::foldedRegions() const;` (replaces `QMap<int, QStringList>`) | W2 → W1 | W0 changes the signature to return the current keys and drops `.keys()` at `session.cpp:148`. T2.3 replaces the body. |
| `QList<Editor*> MainWindow::allEditors() const;` `Editor* MainWindow::openFile(const QString& = QString());` (§4) | W3 → W1 | W0-implemented. `openFile` returns the existing editor when the file is already open, and null on failure or a cancelled dialog. It stays a public slot: moc accepts the forward-declared `Editor*` return, and the string connection `SLOT(openFile())` in `toolbarmanager.cpp:26` still resolves (both verified with a scratch build). |
| `void MainWindow::showNotification(const QString&);` `void NotificationBar::showInfo(const QString&);` — informational, no Yes/No buttons | W3 → W1 (restore names an unopenable file) | W0-implemented |
| `void Document::saved(const QString &path, const QByteArray &sha1, qint64 size);` (signal) | W1 internal (Document → DocumentManager) | T1.7 |
| **Encoding names**: `UTF-8`, `UTF-16LE`, `UTF-16BE`, `UTF-32LE`, `UTF-32BE`, each optionally suffixed `-BOM`, plus ICU converter names as `ucnv_getCanonicalName` returns them for `windows-1252`, `Shift_JIS`, `ISO-8859-1`, `ISO-8859-6`, `windows-1256` | TextCodec ↔ Document ↔ MainWindow ↔ cache JSON | T1.3. The UCS-2 BOM items set `UTF-16BE-BOM` and `UTF-16LE-BOM`. "Convert to ANSI" keeps today's `ISO-8859-1`. The v1.0.0 cache values `UTF-8`, `UTF-8-BOM`, `UTF-16LE`, `UTF-16BE` and `ISO-8859-1` are accepted unchanged. |
| Cache entry key `"lineEnding"` (`"LF"`/`"CRLF"`/`"CR"`), optional | W1 internal | T1.5. Additive: 1.0.0 ignores unknown keys. When the key is absent, restore keeps the detected value or the default. |

"Replace N characters" (convert and save-time) is a single undoable edit in the editor that replaces each unmappable position with `?`. The text is then encodable and saves without loss. If undo brings the characters back, the next save returns `EncodeLossy` again, so I3 holds without a Document "allow replace" flag.

#### Data flows

**Save** (`MainWindow::saveFile`, also used by Save All and the close prompts):
1. The action is disabled for read-only and partial-view tabs (T3.2). `Document` refuses them anyway (`ReadOnly`, `PartialView`).
2. `editor->syncToDocument()` copies `rawText()` into the PieceTable. The `QTextDocument` modified flag is untouched.
3. `doc->save(mode)`:
   - normalise to `m_lineEnding`;
   - `TextCodec::encode(text, encoding)`; anything unmappable returns `EncodeLossy` and writes nothing;
   - hard link (`st_nlink > 1`) or owner ≠ euid: direct write, and `lastSaveWasDirect()` is set;
   - otherwise `QSaveFile` write and `commit()`; on failure, `IoError`, `lastError()` and `directWriteAvailable()`;
   - on `Ok`: clear `contentModified` and `formatDirty`, then emit `saved(path, sha1, size)`.
4. DocumentManager records `(mtime, size, sha1)`, and the watcher event the save produces is dropped.
5. MainWindow:
   - `Ok`: `editor->runSyncedEdit(SyncTarget::Clean, {})`, plus the notice if the write was direct.
   - `EncodeLossy`: the dialog offering Save As UTF-8, Replace N, or Cancel.
   - `IoError`: the message, plus "Save directly" only if `directWriteAvailable()`; on confirm, `save(WriteMode::AllowDirect)`.
   - Every other result leaves both flags set.

**Load** (`MainWindow::openFile` → `DocumentManager::openDocument` → `Document::load`):
1. Mode by size: over 50 MB, or minified over 20 MB, is `LargeFile`, a partial view: read-only with an 8 KB probe.
2. Small and Medium: read all bytes → `CharsetDetector::detect(all)` → `TextCodec::decode` → `lossless`. A lossy decode, or a Medium-mode long line, makes the document read-only.
3. Line endings: count them and record whether they are mixed. With no line break, use `Settings::defaultLineEnding()`. Normalise to LF internally.
4. `Editor` constructor: `runSyncedEdit(Clean, setPlainText)`, then `setReadOnly(document->isReadOnly())`.
5. MainWindow notifications: the partial-view banner, or for a lossy decode a message offering Reopen with Encoding…. The mixed-ending notice appears at first save, once per document.

**Cache write** (every 30 s, and in `closeEvent`):
1. For each editor in `allEditors()`, include it if `isModified()`, or if it is untitled and `!QTextDocument::isEmpty()`. Its text is `editor->rawText()`.
2. Add every registered draft that restore did not place, verbatim.
3. Write each entry into `unsaved.new/` with `QSaveFile`, then `fsyncDirectory(unsaved.new)`.
4. Rename `unsaved/` → `unsaved.old/`, rename `unsaved.new/` → `unsaved/`, `fsyncDirectory(sessions)`, delete `unsaved.old/`.

**Restore** (`Session::restoreDrafts`):
1. For each `*.json` in `unsaved/`: if it does not parse, move it to `unsaved.corrupt/` and continue.
2. If `originalFilePath` is set: `Editor *e = openFile(path)`. Accept `e` only if `e->document()->filePath() == path`. Otherwise use `newFile()`, keep `originalFilePath` as the Save As default, and `showNotification` naming the file.
3. Apply the cached encoding and line ending, then `runSyncedEdit(Modified, setPlainText(text))`.
4. Register the entry as placed.

**Startup ordering** (`main.cpp`):
1. `QApplication` and the organisation/application names. These must come first because `AppDataLocation` depends on them.
2. Parse the arguments, so `--help` and `--version` never touch the lock.
3. `mkpath` the data and config directories.
4. `SingleInstance::acquire()`. On failure, `forward(absolute files, goto)` and exit 0 on ack, 1 on timeout. `--new-window` is forwarded the same way (Q2).
5. `removeServer` and `listen`.
6. `Session::recoverCacheDirectories()`.
7. `Settings::load()`, then the theme. Settings load moves after the lock because D10's corrupt-copy is a write.
8. Construct and show `MainWindow`; load plugins (as today).
9. `restoreDrafts`, then `restoreFiles` if the preference is set and `session.json` parses, then the file arguments and `--goto`. Close the initial empty Untitled tab as today.
10. Connect `openRequested` to opening the files, `goToLine`, `raise()` and `activateWindow()`.
11. `exec()`. The lock is released when the process exits.

#### Threading

Everything stays on the GUI thread. No new threads, no new `QtConcurrent` use. The existing `LargeFileReader` index thread is untouched.

- **Save.** `QSaveFile` write, commit and rename block the GUI thread as today's `QFile` write does. The size is bounded at 50 MB because partial views cannot be saved.
- **Cache write.** It already runs on the GUI thread every 30 s. Phase 1 adds two directory fsyncs, and T1.5 reports the measured wall time of one write with 10 drafts.
- **Load.** Whole-file detection and the lossless round trip run inside the load, which is already synchronous. A Medium file's transient peak is roughly bytes + UTF-16 text + re-encoded bytes (about 4× the file size). T1.3 reports load time and peak RSS for a generated 45 MB file before and after.
- **Why not a worker.** Moving load to a worker would require a thread-safe `Document`, a QObject that emits signals into widgets. No current constraint justifies that (P1, P5).
- **Single instance.** The holder side is event-driven (`newConnection`). The client side blocks only before the window exists.

#### Dependency: ICU required

Today's `CMakeLists.txt` runs `find_package(ICU QUIET COMPONENTS uc data)` and defines `HAVE_ICU=1`, but nothing in `src/` references `HAVE_ICU`: ICU is linked and never used. TextCodec is its first real use. T0.2 makes it required and removes the unused define:

```cmake
find_package(ICU 70 REQUIRED COMPONENTS uc data)
target_link_libraries(prime-edit-core PUBLIC ICU::uc ICU::data)
```

`ucnv` lives in `libicuuc`. Local: ICU 70.1, headers in `/usr/include`, libraries in `/usr/lib/x86_64-linux-gnu`; `CMakeCache` confirms these, not the miniforge 78.2 headers. CI (`ubuntu-24.04`) gains `libicu-dev` in the apt line. The `.deb` gains `libicu70` in `Depends`. The 1.0.0 binary already links `libicuuc.so.70` and `libicui18n.so.70` (checked with `ldd`) without declaring them.

#### Invariants and where they are enforced

| Invariant | Enforced by |
|---|---|
| I1 round trip | TextCodec (BOM and exact codec); Document (detection on the whole file, D9 default, mixed-ending record); `Editor::rawText()` (D11); folding never edits text (Editor, D2); proven end-to-end by T4.1 through `MainWindow::saveFile` |
| I2 modified tracking | `Editor::runSyncedEdit` (both flags, per-site target); `Document` format-dirty flag; MainWindow clears the flags only on `SaveResult::Ok`; Session and the close prompts read `isModified()` only |
| I3 no lossy write without consent | TextCodec (`lossless`, `unmappable`); Document (`EncodeLossy`, `PartialView`, `ReadOnly`, read-only on a lossy decode); Editor mirrors read-only; MainWindow consent dialogs that name the count |
| I4 durable writes | Document (`QSaveFile`, the direct-write rules); Session (atomic set replacement, `session.json` and named sessions); Settings and MacroRecorder (`QSaveFile`, corrupt-copy); `FileUtils::fsyncDirectory` |
| I5 unsaved work survives | SingleInstance (one writer per profile); `Session::recoverCacheDirectories` (recovery table); `restoreDrafts` (every launch, untitled fallback, corrupt entries kept); the cache writer (`allEditors`, `QTextDocument` content check, unplaced drafts kept); the 30 s timer |

### 9.2 Breakdown

#### Phases and ownership

Every file has exactly one owner per phase. A file not listed is frozen in Phase 1. A change it turns out to need goes back to the orchestrator, which reassigns it.

| File(s) | Phase 0 | Phase 1 |
|---|---|---|
| `src/core/document.{h,cpp}` | T0.1 uthman | W1 uthman |
| `src/core/textcodec.h` | T0.1 uthman (declarations) | W1 uthman |
| `src/core/textcodec.cpp`, `src/core/singleinstance.{h,cpp}` (new) | — | W1 uthman |
| `src/core/{charsetdetector,documentmanager}.{h,cpp}`, `src/core/macrorecorder.cpp`, `src/utils/{settings,fileutils}.{h,cpp}`, `src/main.cpp` | frozen | W1 uthman |
| `src/core/session.{h,cpp}` | T0.1 uthman (`.keys()` call site) | W1 uthman |
| `CMakeLists.txt` (top level) | T0.2 abdullah | W1 uthman (adds `textcodec.cpp` and `singleinstance.cpp` only) |
| `src/ui/editor.{h,cpp}` | T0.1 uthman | W2 ali |
| `src/ui/mainwindow.{h,cpp}`, `src/ui/mainwindow_extended.cpp`, `src/ui/notificationbar.{h,cpp}` | T0.1 uthman | W3 ali |
| `src/ui/{statusbar,preferencesdialog}.{h,cpp}`, `src/ui/markdown{mode,outline,preview}.{h,cpp}`, `html-docs/guide.html`, `html-docs/faq.html` | frozen | W3 ali (the markdown files only if a forced change appears) |
| `tests/validation/test_all_features.cpp` | T0.1 uthman (`bool` → `SaveResult`) | frozen |
| `tests/validation/crash_test.cpp`, `tests/feature/*` | frozen | frozen (T1.5 must turn the known crash_test case green without editing the file) |
| `tests/CMakeLists.txt`, `tests/regression/CMakeLists.txt`, `tests/regression/support.h`, `tools/check_function_length.py`, `.github/workflows/build.yml` | T0.2 abdullah | frozen |
| `tests/regression/test_<task>.cpp` | T0.2 abdullah (stubs) | the task named in the file |
| `tests/regression/test_i1_roundtrip.cpp` | T0.2 abdullah (stub) | W4 abdullah |

Phase 1 runs W1, W2 and W3 in parallel, one git worktree each (W2 and W3 are separate ali spawns). W4 starts when its dependencies are met. Every dependency below means "merged into the dependent workstream's branch". The orchestrator merges each finished task's commits into the downstream worktree, and since the files are disjoint those merges cannot conflict.

#### Phase 0 — W0 (T0.1 and T0.2 in parallel, disjoint files)

**T0.1 — interfaces and forced call sites.** Agent: **uthman**. Depends on: none (base `9f5a683`). Suite: the existing `test_runner`, `crash_test` and `tst_markdown`.
- `document.h/.cpp`: `SaveResult`, `WriteMode`, `save`/`saveAs` returning `SaveResult` (a map of today's behaviour), `lastError`, `isPartialView` (`fileMode() == LargeFile`), `isFormatDirty` (returns false until T1.2), and the declarations of every T1.x contract above.
- `textcodec.h` plus a stub `textcodec.cpp`: `decode` returns `lossless=false`, and `encode` reports every character as unmappable. W1 owns `textcodec.cpp` from T1.3 on.
- **Placeholder bodies (R2-1).** Every contract declared in W0 but defined by a later task gets a 1.0.0-behaviour placeholder body (`false`, empty, or a no-op), so suites link at the W0 base and command 4 yields real test failures, not link errors.
- `editor.h/.cpp`: `SyncTarget`, `runSyncedEdit` and `rawText`, both final and unused; `foldedRegions()` returns `QList<int>`.
- `session.cpp:148`: drop `.keys()`.
- `mainwindow.h/.cpp`: `allEditors()`, `Editor* openFile()`, `showNotification()`. In `saveFile` and `saveFileAs`, test `== SaveResult::Ok`, and on `Ok` call `editor->runSyncedEdit(SyncTarget::Clean, {})`. That is a no-op today, because `syncToDocument` already cleared the flag, and it lets T2.2 stop clearing it before the save without leaving W2's branch inconsistent.
- `notificationbar.h/.cpp`: `showInfo()`.
- `test_all_features.cpp:183,887,919`: adapt to `SaveResult`.

Every other caller of `openFile` (`main.cpp:84`, `session.cpp:184,310`, `lspbridge.cpp:104`, the plugin adapter in `mainwindow.cpp`, the `MarkdownModeController::openFileRequested` lambda) ignores the return value and is left unchanged.

**Behaviour-preserving:** after T0.1, `test_runner` and `tst_markdown` pass, and `crash_test` fails on exactly the one known case ("Untitled with content, switch theme…"), the same result as at `9f5a683`.

**T0.2 — build and test scaffold, CI.** Agent: **abdullah**. Depends on: none. Suite: all 16 stubs.
- Top-level `CMakeLists.txt`: ICU required (above).
- `tests/CMakeLists.txt`: `add_subdirectory(regression)`.
- `tests/regression/CMakeLists.txt` registers the 16 suites listed in this section by explicit name, one executable per file. Each links `prime-edit-core` and `Qt::Test`, adds `../validation` to its include path (for `test_isolation.h`), and runs with the environment the validation suites use plus `XDG_RUNTIME_DIR=<binary dir>/home-<suite>/run` and `PRIMEEDIT_BIN=$<TARGET_FILE:prime-edit>`.
- `support.h`: a temp-dir fixture, `writeBytes`/`readBytes`, and a call to `isolateFromUserData()`.
- Each `test_<task>.cpp` stub is a QtTest with one slot that runs `QSKIP("pending <task>")`.
- `tools/check_function_length.py`: Python stdlib only. It takes `--base <rev>` and reports every C++ function in `src/` that the diff adds or modifies and that is longer than 50 lines; exit 1 if any. No length tool is installed (`lizard` and `clang-tidy` are absent), so this check is built, and its DoD requires it to be probed with one planted 51-line function and one 50-line function.
- `.github/workflows/build.yml`: add `libicu-dev`; add an `asan` job (Debug, `-fsanitize=address,undefined -fno-omit-frame-pointer`, `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`, full `ctest`).

At `0e79f0f` both configurations are already clean of sanitizer reports. The scratch build run for this section found 0 `AddressSanitizer`/`LeakSanitizer`/`runtime error` lines, and the only failure was the known `crash_test` case. So the new job is red only for that case until T1.5.

**Phase 0 gate.** T0.1 and T0.2 merge into `fix/phase1` (cut from `9f5a683`). That merge commit is **the W0 base**, and every Phase 1 worktree branches from it.

#### Phase 1 — W1 core (agent: **uthman**; tasks in this order)

| Task | D | Scope | Depends on | Suite |
|---|---|---|---|---|
| T1.1 | D7, D3 core, D12 core | `QSaveFile` save; `PartialView`/`ReadOnly`/`IoError`; `WriteMode::AllowDirect`, `directWriteAvailable`, `lastSaveWasDirect`; the hard-link, foreign-owner and 0444 rules; `setEncoding`/`setLineEnding` refuse on a partial view; `lastError`. Confirm with `strace -f -e trace=fsync,fdatasync,rename` whether `QSaveFile::commit` fsyncs the file, and add an explicit fsync before commit if it does not. | W0 | `test_t1_1_save`: rlimit ENOSPC child, 0444, a writable file in a 0555 dir with `AllowDirect` on retry, hard link kept, symlink and mode kept, 51 MB and 21 MB-minified files give `PartialView` with bytes unchanged |
| T1.2 | D1 core, D9, D11 core | Format-dirty flag (`isModified = content ∥ format`, cleared only by a successful save or load); `hadMixedLineEndings`; a file with no line break uses `Settings::defaultLineEnding()` | T1.1 | `test_t1_2_flags`: conversion stays modified after type plus undo (with an Editor); D9 bytes `61 0A 62`; mixed-ending flag and majority normalisation |
| T1.3 | D6 core | `textcodec.cpp` (adds it to `CMakeLists.txt`); detector on the whole file returning `{name, hasBom}`; decode through TextCodec in Small and Medium mode; read-only on a lossy decode; `wasDecodedLossily`; `reloadWithEncoding`; `EncodeLossy` at save; BOM kept; the encoding-name table; `setSizeThresholdsForTest` | T1.2 | `test_t1_3_encoding`: non-UTF-8 byte after 8 KB, Medium-mode UTF-16, lossy decode opens read-only, `reloadWithEncoding` recovers Windows-1252, BOM kept on save, CP932 many-to-one opens read-only, `EncodeLossy` leaves the file untouched |
| T1.7 | D8 core | `Document::saved` signal; DocumentManager records the save and suppresses the watcher event it causes | T1.1 | `test_t1_7_watch`: own save emits no `fileExternallyModified`; an external write does |
| T1.4 | D10 | `QSaveFile` for `settings.json`, `session.json`, `sessions/*.json` and `macros.json`; corrupt-copy and no-write-until-change | W0 (runs after T1.7 for sequencing) | `test_t1_4_persistence`: each of the four truncated files gives a `.corrupt-*` copy, the original unchanged, and no write until a setting changes |
| T1.5 | D4 cache and restore, D5 core, D6 restore | `recoverCacheDirectories`, `restoreDrafts`/`restoreFiles`, the cache writer (`allEditors`, `QTextDocument` content check, `rawText`, unplaced drafts kept, atomic replacement, `fsyncDirectory`), corrupt entries moved to `unsaved.corrupt/`, the `lineEnding` key, `Session::save` over `allEditors` | T1.3, T1.4 | `test_t1_5_cache`: each recovery-table state built on disk; unreadable original gives a new tab and the active tab unchanged; corrupt entry survives the next write; restored draft stays modified after type plus undo; cached encoding applied on restore; modified untitled doc in pane 2 is cached on quit. Plus the existing `crash_test` going fully green. |
| T1.6 | D4 single instance and startup | `singleinstance.{h,cpp}` (adds them to `CMakeLists.txt`); `main.cpp` in the order above | T1.5 | `test_t1_6_instance`: runs `$PRIMEEDIT_BIN` only, with HOME, XDG_DATA_HOME, XDG_CONFIG_HOME and XDG_RUNTIME_DIR all in scratch. Covers: forward with an absolute path exits 0 and the holder's drafts remain; a holder that never acks makes the second launch exit 1; a launch with file arguments restores drafts; a corrupt `session.json` still restores drafts. |

**T1.8 (added 2026-11-27, found by the T4.1 matrix).** D6/D11. A BOM-less UTF-16 file with CRLF endings is detected as ISO-8859-1. That decode is lossless, but the NULs between CR and LF make `detectLineEnding` report mixed endings, so D11 normalises every CR to LF and doubles each line break on save. Fix in two layers:
- `CharsetDetector` recognises BOM-less UTF-16 by its NUL pattern: a high share of NUL bytes concentrated at even or odd offsets.
- **Safety net:** if the decoded text contains a NUL next to a CR or LF, the document opens read-only (D12) with the lossy-load notification offering Reopen with Encoding, and the D11 normalisation never runs on it.

Agent: uthman. Depends on: T1.6. Suite: the 4 failing `test_i1_roundtrip` cases, `byDetection(UTF-16LE/CRLF)`, `(UTF-16BE/CRLF)` and the two Medium-mode cases, must pass. Add a T1.3-suite case for the safety net, using a NUL pattern the detector does not catch.

#### Phase 1 — W2 editor (agent: **ali**; tasks in this order)

| Task | D | Scope | Depends on | Suite |
|---|---|---|---|---|
| T2.1 | D1 editor | RTL pass through `runSyncedEdit(Preserve)`, only for Arabic or previously-RTL blocks; load through `runSyncedEdit(Clean)` | W0 | `test_t2_1_modified`: RTL pass then type gives modified, Save enabled and the draft cached, for Latin and for Arabic content (uses a MainWindow) |
| T2.2 | D7 editor, D11 | `syncToDocument` uses `rawText()` and leaves the `QTextDocument` flag alone | T2.1 | `test_t2_2_sync`: NBSP and U+2028 survive type, delete and save; a failed save leaves the editor modified |
| T2.4 | D3 editor, D12 editor | No banner text appended to a partial view; `setReadOnly` mirrored at bind time and on `readOnlyChanged` | T2.2 | `test_t2_4_readonly`: edits rejected on 51 MB and 21 MB-minified files, and on a document made read-only after binding |
| T2.3 | D2 | View-only folding, `FoldHeader` user data, derived state, the unfold rules (cursor, Find, Go to Line, multi-cursor; deleting a header; undo) | T2.4 | `test_t2_3_folding`: fold, type and save keeps the full text; Find inside a fold unfolds before the replace; deleting a header shows its body; session round trip of fold state |

#### Phase 1 — W3 main window (agent: **ali**; tasks in this order)

| Task | D | Scope | Depends on | Suite |
|---|---|---|---|---|
| T3.1 | D5 UI | Remove Clone (menu, `cloneToOtherView`, the clone in `splitView`); route close by the sender's tab widget; lifetime guard in `closeFile` over `allEditors()`; settings handlers over `allEditors()` | W0 | `test_t3_1_split`: a settings change reaches the pane-2 editor; a close from tab bar 1 with pane 2 focused closes the pane-1 tab; Clone absent |
| T3.2 | D7 UI, D3 UI, D12 UI | `SaveResult` handling, "Save directly" only if `directWriteAvailable()`, the direct-write notice, flags cleared only on `Ok`; Save, Save As, Save All and conversion actions disabled per tab for partial views (menu and status bar); partial-view banner via `showInfo`; both close prompts key on `isModified()` | T1.1, T2.2, T3.1 | `test_t3_2_save_ui`: still modified after a failed save plus type plus undo; 0555 dir offers direct write and saves on confirm; Ctrl+S and conversions leave a partial view's bytes unchanged; a modified doc that later became read-only prompts on close |
| T3.3 | D6 UI, D11 UI | Convert preflight with count and the first 5 positions as line:column, plus Replace; UCS-2 items mapped to `UTF-16xE-BOM`; Reopen with Encoding… in the menu and in the lossy-load notification; the save-time `EncodeLossy` dialog; mixed-ending notice once per document | T1.3, T1.2, T2.4, T3.2 | `test_t3_3_encoding_ui`: UCS-2 items write a BOM; Convert to ANSI with Arabic shows the prompt and writes nothing on Cancel; a lossy-decoded document refuses keystrokes; Reopen with Encoding from the menu recovers Windows-1252; mixed-ending notice shown once |
| T3.4 | D8 UI, D1 reload | Reload bar with two actions; background tabs get the bar on activation; reload through `runSyncedEdit(Clean)` for the document's editor; `reloadFromDisk` the same | T1.7, T3.3 | `test_t3_4_reload`: own save shows no bar; external change plus unsaved edits shows the bar and does not reload silently; a background tab reloads on activation |
| T3.5 | docs, Auto-save preference | Hide the Auto-save preference. Update `html-docs/guide.html` and `faq.html` for every §6 behaviour, plus always-restored drafts and the `.corrupt-*` copies. The current pages describe Clone and the old cache behaviour. | every T1.x, T2.x, T3.x | none (a docs and UI-copy task); DoD checked by grep |

#### Phase 1 — W4 round-trip matrix (agent: **abdullah**)

| Task | Scope | Depends on | Suite |
|---|---|---|---|
| T4.1 | The §5 I1 matrix through `MainWindow::saveFile`: every fixture and line ending listed there, legacy fixtures both by detection and by Reopen with Encoding, Medium mode via `setSizeThresholdsForTest` | T1.3, T2.2, T3.3 | `test_i1_roundtrip` |

#### Coverage check against §5

Every §5 bullet has exactly one owning suite:

- D1: T2.1, T1.5 (restored draft), T1.2 (conversion).
- D2: T2.3.
- D3: T1.1 (Document), T2.4 (edits), T3.2 (Ctrl+S and conversions).
- D4: T1.5 (recovery table, unreadable original, corrupt entry, `crash_test`), T1.6 (forwarding, no ack, file arguments, corrupt `session.json`).
- D5: T1.5 (pane-2 cache), T3.1 (the rest), plus the ASan DoD.
- D6: T1.3 (detection, Medium UTF-16, lossy, CP932, BOM, reload API), T1.5 (restore), T3.3 (UCS-2, ANSI prompt).
- D7: T1.1 (Document cases), T3.2 (type plus undo, direct-write confirm).
- D8: T1.7, T3.4.
- D9: T1.2.
- D10: T1.4.
- D11: T2.2, T1.2 plus T3.3 (mixed endings).
- D12: T3.3 (lossy refuses keystrokes), T3.2 (prompt).
- I1: T4.1.
- CI: T0.2.

### 9.3 Verification commands

Each builder sets `S=<its own session scratchpad>/<task id>`, `B=$S/build`, `A=$S/asan`. No build directory is ever under the repository or the user's profile. Tests never run `/usr/bin/prime-edit`. Every process test uses `$PRIMEEDIT_BIN` with scratch HOME and XDG directories.

1. **Task suite:** `cmake -S . -B $B -DOPENGL_INCLUDE_DIR=/usr/include && cmake --build $B -j$(nproc) && ctest --test-dir $B --output-on-failure -R '^<suite>$'`
2. **Full ctest:** `ctest --test-dir $B --output-on-failure`
3. **ASan and UBSan:** `cmake -S . -B $A -DOPENGL_INCLUDE_DIR=/usr/include -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" && cmake --build $A -j$(nproc) && UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ctest --test-dir $A --output-on-failure 2>&1 | tee $A/ctest.log; grep -c 'ERROR: AddressSanitizer\|ERROR: LeakSanitizer\|runtime error:' $A/ctest.log` (expect `0`)
4. **Fails before the fix:** `git worktree add $S/before <W0 base>`, copy the task's suite file over the stub there, then run command 1 with `-S $S/before -B $S/before-build`. Expect every new test function except helpers to FAIL, and paste the names. The W0 base stands in for `0e79f0f` because the new suites need W0's declarations to compile, and T0.1's DoD proves the W0 base behaves exactly like `9f5a683`.
5. **Function length:** `python3 tools/check_function_length.py --base <W0 base>` (expect exit 0)
6. **No new debug output:** `git log --no-merges -p --format= <W0 base>..HEAD -- src | grep -E '^\+.*\b(qDebug|printf|std::cout|std::cerr)\b'` (expect no output)
7. **Docs** (T3.5 only): `grep -n -i 'clone to other view' html-docs/*.html` gives no output, and `grep -n -i 'reopen with encoding\|already running\|read-only' html-docs/guide.html html-docs/faq.html` shows the new text.

T0.1 and T0.2 run 1 to 3 and 5 to 6. T0.1 runs no command 4 (it has no new tests). Its DoD instead pastes the full-ctest result showing exactly the known `crash_test` failure, before and after.

### 9.4 Definition of done (every task)

- [ ] Its regression tests fail on the W0 base (command 4, output pasted) and pass after (command 1). For T0.1: the existing suites give the same result as at `9f5a683`.
- [ ] Full ctest is green (command 2). The one exception: until T1.5 merges, `crash_test` may fail on its single known case and on nothing else.
- [ ] ASan and UBSan clean: command 3 prints `0`, with the same `crash_test` exception.
- [ ] Every function added or modified is at most 50 lines (command 5). **Ruling (orchestrator, 2026-11-27, T0.2 finding):** about 78 functions were already over 50 lines at the base. Such a function may be edited only if it does not grow, and new logic goes into new helpers of 50 lines or fewer. Splitting the legacy functions is a Phase 3 refactor, because a forced split inside a data-loss fix adds risk to the code being made safe. `tools/check_function_length.py` enforces this.
- [ ] No new `qDebug`, `printf` or `std::cout`/`cerr` (command 6).
- [ ] html-docs updated where user-visible behaviour changes. Only W3 owns `html-docs/` in Phase 1. A W1, W2 or W4 task whose change is user-visible writes the user-facing description in its receipt for T3.5. W3 tasks T3.1 to T3.4 update the pages for their own behaviour. T3.5 covers the rest.
- [ ] Only the files this section assigns to the task are changed. Scope is measured over the task's own non-merge commits: `git log --no-merges --name-only --format= <W0 base>..HEAD | sort -u` lists nothing else. Dependency merges are excluded, and commands 5 and 6 use the same base.
- [ ] The receipt lists the contracts the task produced or consumed, each grepped in producer and consumer.

### 9.5 Integration and install

1. **Merge.** The orchestrator merges W1, W2, W3 and W4 into `fix/phase1` after each task's output-gate verdict is recorded, and runs commands 1 to 3 on the merge. Abu Bakr runs the integration check:
   - every contract in §4 and §9.1 grepped in producer and consumer;
   - `grep -rn 'QSKIP("pending' tests/regression` gives no output (no stub survives);
   - the `crash_test` exception is gone: full ctest is green under both configurations.
2. **ASan CI job.** Added by **abdullah in T0.2**. It must be green on the merged branch before the PR is opened.
3. **Version bump** (abdullah, integration task I1): `project(PrimeEdit VERSION 1.0.1 ...)` in `CMakeLists.txt`, and `main.cpp` reads `PROJECT_VERSION` through a compile definition instead of the hard-coded `"1.0.0"` at `main.cpp:21`, so `dpkg -i` sees an upgrade. Verified by command 1 plus `$B/PrimeEdit --version` in a scratch HOME.
4. **Rollback artifact** (abdullah, I1). `release/` is gitignored, and `release/prime-edit_1.0.0_amd64.deb` is the only copy of the rollback package. Before anything else: copy it to `~/primeedit-rollback/prime-edit_1.0.0_amd64.deb` and check `sha256sum` = `d3e9c9e94341152ae53c25694780bdcbe1b81a95a9798c8142034110f72dd82f`.
5. **Rollback rehearsal from §7** (abdullah, integration task I2). All in `$S/rehearsal`, with `HOME`, `XDG_DATA_HOME`, `XDG_CONFIG_HOME` and `XDG_RUNTIME_DIR` set to scratch directories and `QT_QPA_PLATFORM=offscreen`:
   1. Run the new `$B/PrimeEdit`, create two drafts (one untitled, one modified file), wait for one 30 s cache write, then kill it with `SIGKILL`. Create an empty `sessions/unsaved.old/`.
   2. `dpkg-deb -x ~/primeedit-rollback/prime-edit_1.0.0_amd64.deb $S/rehearsal/v100`. This extracts without installing; never `dpkg -i` here.
   3. Run `$S/rehearsal/v100/usr/bin/prime-edit` under the same scratch environment for 35 s, then `SIGKILL` it. Because 1.0.0 deletes cache files on restore and rewrites the set after 30 s, a `sessions/unsaved/` holding both draft texts proves 1.0.0 restored them into editors. Paste the before and after listing and the text diff.
6. **`.deb` build** (abdullah, I2). There is no packaging script. Reproduce the 1.0.0 payload layout exactly as `dpkg-deb -c` shows it, in a fresh staging directory `$S/deb` (never `release/deb`, which holds the 1.0.0 staging tree):
   - `usr/bin/prime-edit`: the Release build's `PrimeEdit`, renamed;
   - `usr/share/applications/prime-edit.desktop`: copied from `release/deb/...`, not from `resources/`, because the two differ and the installed launcher must not change;
   - `usr/share/icons/hicolor/256x256/apps/prime-edit.png`: identical to `resources/icons/prime_edit_icon.png`;
   - `DEBIAN/control`: copied from `release/deb/DEBIAN/control` with `Version: 1.0.1` and `libicu70` added to `Depends`.

   Build with `dpkg-deb --root-owner-group --build $S/deb $S/prime-edit_1.0.1_amd64.deb`. Verify with `dpkg-deb -I` and `-c`, and `ldd` on the staged binary.
7. **Install** happens only after the user confirms the §7 pre-install procedure in their own message. The orchestrator hands the user the `.deb` path and the procedure. No agent runs `dpkg -i`, launches `/usr/bin/prime-edit`, or reads or writes `~/.local/share/PrimeEdit` or `~/.config/PrimeEdit`.

## 10. Round-1 resolution index

| Finding | Resolution |
|---|---|
| R1-1 | Clone removed (D5), shared `QTextDocument` dropped, true Clone deferred to Phase 3 |
| R1-2 | §9 re-partition by Abu Bakr |
| R1-3 | D4 Restore: openFile returns Editor*, untitled fallback, corrupt entries kept, writer never drops unplaced drafts |
| R1-4 | D1 SyncTarget per site, format-dirty flag, restore applies encoding |
| R1-5 | D7 unsafe-atomic cases, flag cleared only after Ok, rlimit test |
| R1-6 | D4 per-profile socket path, startup order, IPC with ack and timeouts, full recovery table, dir fsync, `--goto` |
| R1-7 | D11 added: toRawText, NBSP and U+2028 preserved, mixed-EOL policy; matrix through the real path |
| R1-8 | D6 definition of lossless, Save-time EncodeLossy UI, UTF-16 index unit |
| R1-9 | §5 named test per sub-defect; D9 test fixed; D10 covers all files; crash_test root cause named in D4 |
| R1-10 | D12 added; D3 covers minified files over 20 MB, with a 21 MB test |
| R1-11 | Header: tier T3 and fixes_artifact; §9 architecture by Abu Bakr |
| R1-12 | §7 pre-install procedure |
| R1-13 | D2 derived fold state, unfold rules; the session.json fold format is unchanged; foldedRegions() becomes QList<int>, call site session.cpp:148 changed in W0 |
| R1-14 | Out-of-scope reasons; I5 states the 30 s bound |
| R1-15 | No reference counting (allEditors scan); fixed directory names under the lock |
| R1-16 | Base `0e79f0f`, harness merges first; rollback is the .deb, rehearsed |
| R1-17 | D10: copy, keep the original; named sessions in I4 |

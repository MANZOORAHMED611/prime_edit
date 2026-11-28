# Spec: PR #5 review findings — direct-write safety, draft-cache failure and restore races, decode peak memory (P1.11)

**Status:** approved — Umar round 2 (scoped re-check). Round-2 minor notes R2-1–R2-3 applied after approval.
**Tier:** T2. Touches user files (direct in-place writes) and stored data (the draft cache `sessions/unsaved/`).
**fixes_artifact:** escapes from Phase 1 workstreams the Phase 1 output gate validated (`2026-11-27-phase1-output-gate.md`): T1.1 direct write (`39e093f`, F1), T1.3 TextCodec (`b32d54b`, F5), T1.5 draft cache and restore (`0fce977`, F2/F3), T1.9 `placeDraft` (`ec3eacd`, F4). All four ship in prime-edit 1.0.1 (`prime-edit_1.0.1_amd64.deb`, sha256 `84ae8a12…d1e3`, built from `bfc7102`).
**Parent spec:** `2026-11-26-phase1-data-loss-fixes.md` (invariants I1–I5)
**Source:** the automated Claude review comment on PR #5 (https://github.com/MANZOORAHMED611/prime_edit/pull/5), 2026-11-27. Every finding below was re-verified against the code by the orchestrator before this spec was written; file:line references are at the base commit.
**Base:** branch `phase1/release-1.0.1` at `fdd5867`. "Fails before the fix" means fails on `fdd5867`.

## 1. Objective

Close five defects, four of which can lose user data:

| # | Defect (verified) | Where | Loss |
|---|---|---|---|
| F1 | Direct in-place save (hard link, foreign owner) opens with `Truncate` and then writes. A write failure part-way (ENOSPC, EFBIG, EIO) leaves the original truncated/partial, and `reportSaveFailure` then tells the user "The file on disk is unchanged." The user-confirmed "Save Directly (Not Atomic)" path (`writeAtomic` with `QSaveFile::setDirectWriteFallback(true)`) has the same truncate-then-write behaviour and the same false message. | `src/core/document.cpp:259-272`, `:241`; `src/ui/mainwindow.cpp:984-988` | Original file contents |
| F2 | `Session::saveUnsavedDocuments` returns `void` and drops `replaceCacheSet`'s `false`. `MainWindow::closeEvent` then accepts the close with no prompt (the quit path never asks to save), so a draft-cache write failure at quit (full disk, unwritable cache dir) discards every unsaved edit. The 30 s tick fails equally silently. | `src/core/session.cpp:470-483`; `src/ui/mainwindow.cpp:1197-1209` | All unsaved edits |
| F3 | The 30 s draft timer is started in the `MainWindow` constructor (`mainwindow.cpp:222-227`), before `restoreDrafts`. `restoreDrafts` calls `readCacheEntries({})`, which marks **every** cached entry seen before any is placed. A tick during a modal dialog inside restore (e.g. `openFile`'s "Could not open file" warning, `mainwindow.cpp:819`) runs `saveUnsavedDocuments`, which skips all seen entries, caches only the drafts placed so far, and `replaceCacheSet` swaps in that subset. Drafts not yet placed are deleted from disk; a crash before restore finishes loses them. | `src/core/session.cpp:353-383, 470-483` | Unplaced drafts |
| F4 | `placeDraft` puts a draft into the editor `openFile` returns even when that document opened read-only (lossy decode, long-line medium file). `Document::saveAs` refuses every read-only document (`document.cpp:185-204`), so the restored draft cannot be saved anywhere. | `src/core/session.cpp:411-449` | Draft unsaveable (text only recoverable by copy-paste) |
| F5 | `TextCodec::decode` holds the input bytes, the decoded text and a full re-encode of the text at once to prove the decode lossless. | `src/core/textcodec.cpp:185-188` | Memory only |

**Measured F5 baseline** (orchestrator, 2026-11-27, standalone probe linking `textcodec.cpp`, `getrusage` `ru_maxrss`, 99 MiB input):

| Input | Peak before decode | Peak after decode |
|---|---|---|
| UTF-8 | 107 MiB | **404 MiB** |
| windows-1252 | 107 MiB | **407 MiB** |

(The probe reports `ru_maxrss/1024`, i.e. MiB.) Target: **≤ 320 MiB** for both, and for a 99 MiB UTF-8 input with a BOM (expected ≈ 107 + 199 text + one slice ≈ 306–310 MiB). The windows-1252 decode buffer reports 399 MiB capacity, but it is allocated `Qt::Uninitialized` and only the ~200 MiB actually written is resident, so it is **excluded** (address space, not RSS).

## 2. Scope

In: F1–F5 as designed in §3; the user-visible failure messages they change; docs where a message or behaviour is documented.

Out:
- Crash-consistency of a direct in-place write (a `kill -9` mid-write still leaves a mixed file). This is inherent to writing in place, is already disclosed by the post-save "written in place … not atomically" notification and by the confirm dialog, and the draft cache still holds the new text.
- Changing which documents are read-only, or letting Save As write read-only documents in general (a LargeFile-mode document holds no text in `m_content`; Save As from it would write an empty file).
- The 30 s interval itself, and the draft-cache file format (unchanged, so no migration).

## 3. Design

### 3.1 F1 — direct writes never truncate first, and report truthfully

Replace `Document::writeDirect` with a reserve → overwrite → trim → restore-on-failure sequence. Every direct in-place write goes through it: the hard-link / foreign-owner path **and** the user-confirmed `WriteMode::AllowDirect` path (`writeAtomic` stops passing `setDirectWriteFallback(true)`; when `QSaveFile::open` fails, the target exists and `mode == AllowDirect`, it calls the new `writeDirect`).

1. Read the current file contents into memory (`original`). If that read fails → `IoError`, file untouched.
2. Open the file `O_WRONLY` **without** `O_TRUNC`.
3. If `data.size() > original.size()`: reserve with the Linux **`fallocate(fd, 0, 0, data.size())` syscall** — never `posix_fallocate`, whose glibc emulation on filesystems without fallocate (NFSv3, FUSE) writes a NUL byte per block past EOF and can leave the file padded (R1-1, reproduced under `strace -e inject=fallocate:error=EOPNOTSUPP`).
   - `EOPNOTSUPP` → continue unreserved; step 5 still protects the original.
   - Any other failure (`ENOSPC`, `EFBIG`, `EDQUOT`, …) → `IoError`. Before classifying, run `ftruncate(fd, original.size())` and check with `fstat` that `st_size == original.size()`: success → `Unchanged`; if the truncate or the check fails → run the step-5 restore and classify as `Restored`/`Damaged` from its result.
4. `pwrite` `data` from offset 0 (loop on short writes), `ftruncate(fd, data.size())`, `fsync(fd)`.
5. If any call in step 4 fails: restore — `pwrite` `original` from offset 0, `ftruncate(fd, original.size())`, `fsync`. The original's blocks are still allocated, so this needs no new space on non-CoW filesystems. If the restore also fails, write `original` to `<cache base>.recovered/<basename>.<uuid>` (sibling of the draft cache) via `FileUtils::writeFileAtomic` and record that path. The cache base (`AppDataLocation + "/sessions/unsaved"`) moves into one helper, `FileUtils::unsavedCacheBase()`, which `Session::unsavedCacheDir()` and `Document` both call, so `core/document` takes no dependency on `Session` (Q2 condition a).
6. `Document` exposes the outcome of the last failed save as `enum class DiskState { Unchanged, Restored, Damaged }` via `lastSaveDiskState()`, plus `recoveryCopyPath()` (empty unless a copy was written). Step 1–3 failures → `Unchanged`; step 5 restore success → `Restored`; restore failure → `Damaged`. Atomic-path failures → `Unchanged`.

`MainWindow::reportSaveFailure` builds its text with a new pure static function `MainWindow::saveFailureMessage(const SaveFailureInfo &)` (struct: display name, `DiskState`, recovery path, error string), so the wording is testable without a Document seam. It picks the sentence from the state:
- `Unchanged` / `Restored`: "The file on disk is unchanged." (as today; `Restored` is byte-identical to the original, which §5 asserts).
- `Damaged` with a recovery copy: "The write failed part-way and '%1' may be incomplete. Its previous contents were saved to %2."
- `Damaged` without a copy: "The write failed part-way and '%1' may be incomplete."

A test-only static hook `Document::setReserveSpaceForTest(bool)` (default true) skips step 3 so tests can force the step-5 path under `RLIMIT_FSIZE`.

### 3.2 F2 — a failed draft write at quit asks before discarding

1. `Session::saveUnsavedDocuments` returns `bool`. It returns **false only when there was something to store** — the entry list (editor drafts plus `m_unplacedDrafts`) is non-empty — **and** `replaceCacheSet` failed. It returns true when the replace succeeded, when restore is in progress (§3.3), and when the entry list is empty even if the replace failed: then the previous on-disk set stays, so stale drafts may resurface on next start, but no data is lost (R1-2). This keeps a full or read-only settings disk from prompting on every quit and logout.
2. `MainWindow::closeEvent`: if it returns false, stop and ask:
   > **Unsaved changes could not be kept** — PrimeEdit could not store your unsaved changes for recovery (the disk may be full or the settings folder unwritable). If you quit now, changes not yet stored for recovery are lost.
   > Buttons: **Cancel** (default, Escape) · **Quit and Discard Changes**
   `closeEvent` records, on entry and before stopping them, which of `m_sessionAutosaveTimer` and `m_draftBackupTimer` were active. On Cancel: `event->ignore()`, restart **exactly those** timers (none if the close arrived before autosave was enabled, e.g. during restore — R1-8), return. On Quit: continue the existing close path. `Session::save` and `Settings::save` still run on the Quit path only.
3. Periodic tick failure: the first failed tick after a success shows `showNotification(tr("Unsaved changes could not be stored for recovery. Free disk space or save your files."))`; repeated failures do not re-notify until a tick succeeds again (a member `bool m_draftCacheFailing`).

### 3.3 F3 — no draft write while drafts are being restored

Layer 2 alone closes F3. Layer 1 is kept for its own reasons (R1-10): no draft write runs inside *any* startup modal — including those in `restoreFiles` and `openRequested`, matching P1.10 §3.4 for the session timer — and F2's Cancel needs a member timer whose state it can record and restore.
1. **Timer gating.** The 30 s draft timer becomes a member `m_draftBackupTimer` (objectName `"draftBackupTimer"`), created but **not started** in the constructor. `setSessionAutosaveEnabled(true)` starts it alongside `m_sessionAutosaveTimer`; `setSessionAutosaveEnabled(false)` stops it; `closeEvent` stops it on its first line together with the session timer.
2. **Restore guard.** `Session` gets `bool m_restoringDrafts`, set for the whole body of `restoreDrafts` (RAII guard so an early return cannot leave it set). While set, `saveUnsavedDocuments` returns `true` without touching disk: the on-disk set is complete and untouched, which is the safe state. This covers any other caller that can run inside a nested event loop during restore (e.g. `closeEvent` reached during a restore modal).

### 3.4 F4 — a draft never lands in a read-only document

In `placeDraft`, after `openFile(original)`: if the returned document `isReadOnly()` or `isPartialView()`, do not use it. Route the draft to `untitledEditorFor(window, original, notice)` with
`notice = tr("%1 opened read-only. Its unsaved text was restored in a new tab.")`. The read-only tab stays open showing the file as on disk. The untitled tab keeps the `originalFilePath` property (Save As default, and re-cached), takes the draft's encoding and line ending as today, and is writable.

### 3.5 F5 — verify losslessness in bounded chunks

`TextCodec::decode` no longer builds the full re-encode. It compares the re-encoding to the input incrementally:
- UTF codecs: one stateful `QStringEncoder` fed `text` in slices of `TextCodec::kVerifyChunk` UTF-16 code units (stateful, so a surrogate pair split across slices encodes correctly); each slice's output is compared with the next bytes of the input; stop at the first mismatch. Lone surrogates still make the result lossy (`loneSurrogates` check unchanged).
- **BOM rule (R1-3b).** For a UTF name that declares a BOM, lossless requires `bytes.startsWith(bom)`, and the comparison starts after it; without the BOM the result is lossy, exactly as today's full re-encode. The decode strips the BOM through a view — `decoder(QByteArrayView(bytes).sliced(n))` — not `bytes.mid(n)`, which copies the whole input in Qt 6 (R1-7).
- ICU codecs: `ucnv_fromUnicode(..., flush=false)` per slice and a final `flush=true` call. **Each call loops while it returns `U_BUFFER_OVERFLOW_ERROR`** — reset the status, compare and drain the target, continue — because pending state, ISO-2022/EBCDIC shift sequences and the flush output can exceed a slice-sized target (R1-3a). Any other `U_FAILURE`, or a from-Unicode callback error → lossy. ICU names ignore the `-BOM` suffix, as `decode` and `encode` do today (R1-3c).
- At the end, the total compared length must equal `bytes.size()` exactly.
`kVerifyChunk` is a `constexpr qsizetype` in `textcodec.h` (value 1 Mi code units) so tests can place a multi-unit character exactly on a slice boundary. `encode` (the save path) is unchanged.

## 4. Contracts

| Symbol | Before | After |
|---|---|---|
| `bool Session::saveUnsavedDocuments(MainWindow*)` | `void` | `bool`; true = cache set replaced, deliberately skipped during restore, or nothing to store (§3.2.1) |
| `Document::DiskState Document::lastSaveDiskState() const` | — | new; meaningful after a non-Ok save |
| `QString Document::recoveryCopyPath() const` | — | new; empty unless F1 step 5 wrote a copy |
| `static void Document::setReserveSpaceForTest(bool)` | — | new, test-only (the only test seam on `Document`) |
| `static QString MainWindow::saveFailureMessage(const SaveFailureInfo &)` | — | new pure function; `reportSaveFailure` uses it; case 4 tests it |
| `QString FileUtils::unsavedCacheBase()` | — | new; single source of the draft-cache path for `Session` and `Document` |
| `TextCodec::kVerifyChunk` | — | new `constexpr qsizetype` |
| `TextCodec::decode` | unchanged signature and results | same `lossless` verdict for every input; lower peak memory |
| `MainWindow` draft timer | local, started in ctor | member `m_draftBackupTimer`, objectName `draftBackupTimer`, started by `setSessionAutosaveEnabled(true)` |
| Draft cache on disk (`sessions/unsaved/*.json`) | — | unchanged format; new sibling `sessions/unsaved.recovered/` only on F1 double failure |

## 5. Test strategy (failing tests first)

Two new suites, both added to `REGRESSION_SUITES`: `tests/regression/test_t1_11a_direct_write.cpp` (cases 1–4, 10; task A) and `tests/regression/test_t1_11b_drafts.cpp` (cases 5–9; task B). Placeholders (declarations returning the old behaviour, e.g. `lastSaveDiskState()` returning `Unchanged`, `saveUnsavedDocuments` returning `true`) are committed first; cases 1–8 must fail on them and the failing output is recorded in the receipt. Disk-full is simulated with `RLIMIT_FSIZE` in a forked child, as `test_t1_1_save.cpp:44-68` already does; unwritable cache with `chmod 0500` on the cache parent inside the isolated test HOME (restored in cleanup).

1. **F1 reserve path.** Hard-linked 1 KiB file; new text 64 KiB; child with `RLIMIT_FSIZE` 4096. Expect `IoError`, `isModified()` true, file bytes **identical** to the original, `lastSaveDiskState() == Unchanged`. (Fails today: file truncated/partial.)
2. **F1 restore path.** Same, with `setReserveSpaceForTest(false)`. Expect `IoError`, original bytes identical, `lastSaveDiskState() == Restored`.
3. **F1 confirmed direct write.** Target in a read-only directory (`readOnlyDirectoryOffersDirectWrite` setup), `save(WriteMode::AllowDirect)` under `RLIMIT_FSIZE` 4096 with 64 KiB new text. Expect original bytes identical. (Fails today: QSaveFile's fallback truncates.)
4. **F1 message wording.** `MainWindow::saveFailureMessage` (pure, §4): for `Damaged` + recovery path, the text contains the path and "may be incomplete" and not "unchanged"; for `Damaged` without a path, "may be incomplete" and not "unchanged"; for `Unchanged` and `Restored`, "unchanged".
4b. **F1 Damaged on the production path (R1-4).** Forked child, no hooks: 8 KiB hard-linked original, new text 6 KiB (smaller, so no reservation), `RLIMIT_FSIZE` 4096. The overwrite fails at 4096, the restore fails at 4096, the 8 KiB recovery copy exceeds the limit too. Expect `IoError`, `lastSaveDiskState() == Damaged`, empty `recoveryCopyPath()`, and `saveFailureMessage` for that state without "unchanged" and with "may be incomplete". The child packs `DiskState` and an empty-path bit into its exit report, as `saveUnderFileSizeLimit` packs its bits.
5. **F2 quit cancelled.** `MainWindow` with one modified untitled tab; `setSessionAutosaveEnabled(true)`; cache parent made unwritable; auto-answer the modal with a zero-delay timer clicking **Cancel**; `close()` returns false, window still visible, `m_sessionAutosaveTimer` and `draftBackupTimer` active again. **5b:** the same without enabling autosave first → after Cancel both timers are **inactive** (R1-8).
6. **F2 quit confirmed.** Same, clicking **Quit and Discard Changes**; `close()` returns true.
6b. **F2 nothing at risk (R1-2).** Cache parent unwritable, no modified tab, no unplaced drafts: `close()` returns true and **no** modal appears (assert with the `PromptAnswerer` missed-count pattern from `test_t3_2_save_ui.cpp`); a tick in that state posts no notification.
7. **F3 restore race.** Cache holds three drafts; the first names a **directory** as `originalFilePath` (readable, so `openFile` is attempted, fails, and shows the modal warning). A zero-delay timer, firing inside that modal, calls `Session::instance().saveUnsavedDocuments(&window)` and then closes the box. After `restoreDrafts` returns: all three drafts are placed (or held unplaced), and before any further save the cache directory still holds three entries. (Fails today: the mid-restore save shrinks the set.)
8. **F3 timer gating.** A fresh `MainWindow`: `findChild<QTimer*>("draftBackupTimer")` exists and `!isActive()`; after `setSessionAutosaveEnabled(true)` it is active with interval 30000.
9. **F4 read-only target.** A file that opens read-only through a normal `openFile` (amended at the output gate: the `test_t1_3_encoding.cpp:140` bytes are read-only only after a forced `reloadWithEncoding`, and detect as ISO-8859-1 on a normal open; use a UTF-16LE file with a BOM and a lone high surrogate, `FF FE 68 00 69 00 00 D8 21 00 0A 00`, lossy through `loneSurrogates`); a cached draft for it whose entry is pinned to **ASCII text with encoding "UTF-8"** (R1-9), so the final `saveAs` result is deterministic. After restore: the file's tab is read-only and shows the file text, not the draft; a separate untitled tab holds the draft text, is not read-only, has `originalFilePath` = the file; `saveAs` of that tab to a new path returns `Ok` and writes the draft text.
10. **F5 boundaries.** Each leg has three variants: lossless → true; one byte altered in the final chunk → false; a trailing extra byte → false.
    - UTF-8 and UTF-16LE (with and without BOM): a surrogate pair split exactly across `kVerifyChunk`.
    - **ISO-2022-JP** (R1-3): an escape-sequence mode change straddling `kVerifyChunk`, and the file ending in JIS mode so the final flush emits `ESC ( B`.
    - **GB18030**: a 4-byte supplementary character split across the boundary.
    - windows-1252: single-byte control.
    - A name declaring UTF-8 with BOM, given bytes **without** a BOM → false.
    Existing `test_t1_3_encoding` and `test_i1_roundtrip` must stay green unchanged.

**Memory receipt (not a CTest).** Rebuild the orchestrator's probe (`/tmp/claude-1000/-home-abusulaiman-dev-repository-prime-edit/5c051adb-108e-4ce7-b900-8902688defd5/scratchpad/memprobe/probe.cpp`, reproduced in the receipt) against the new `textcodec.cpp`; report peak-after-decode for 99 MiB inputs in UTF-8, windows-1252 and **UTF-8 with BOM**. Pass: all three **≤ 320 MiB**, and the probe's `lossless` value is 1 for each.

**strace receipt (R1-1, not a CTest).** Run case 1's scenario (a small driver or the test binary) under `strace -f -e inject=fallocate:error=EOPNOTSUPP` using a small driver (not the case-1 test, whose `Unchanged` assertion is expected to fail under injection), and show: `IoError`, `lastSaveDiskState() == Restored`, file bytes identical (the unreserved path hits the restore — R2-2). Also show `strace` with no injection recording a `fallocate(` syscall, not the `pwrite64` "\0" emulation pattern. This covers the path that no CTest can reach on ext4.

## 6. Blast radius

- `src/core/document.{h,cpp}` — the save path of every direct write. The atomic path (the normal case) changes only by no longer setting `setDirectWriteFallback`.
- `src/core/session.{h,cpp}` — draft cache writes and draft restore.
- `src/core/textcodec.{h,cpp}` — every file open (lossless check).
- `src/ui/mainwindow.{h,cpp}` — `closeEvent`, timer setup, `reportSaveFailure`.
- Existing suites that touch these: `test_t1_1_save`, `test_t1_3_encoding`, `test_t1_5_cache`, `test_t1_10_session_autosave`, `test_t2_4_readonly`, `test_t3_2_save_ui`, `test_i1_roundtrip` — all must pass unmodified, except where a test asserts the old QSaveFile direct-fallback mechanics; any such amendment is declared in the receipt with the reason.
- Docs: `html-docs/guide.html` #recovery gains one sentence for the F2 quit prompt; any doc quoting the failure messages is updated.

## 7. Rollback

Revert the merge commit on `phase1/release-1.0.1`. No on-disk format changes; the optional `sessions/unsaved.recovered/` directory is inert to older builds.

## 8. Breakdown

Two independent tasks, both owner **uthman** (persistence layer), each in its own worktree from `fdd5867`, run in parallel. They touch different functions of `mainwindow.cpp` (`reportSaveFailure` vs `closeEvent`/constructor/`setSessionAutosaveEnabled`); the orchestrator merges both into `phase1/release-1.0.1` and resolves any textual conflict.

**Task P1.11-A — F1 + F5 (write and decode paths).** Branch `fix/p111-direct-write`.
DoD:
- [ ] Placeholders first; cases 1–4b and 10's failing variants fail on them (record output).
- [ ] §3.1 and §3.5 implemented exactly; `setDirectWriteFallback(true)` and `posix_fallocate` used nowhere.
- [ ] Cases 1–4b and 10 in `tests/regression/test_t1_11a_direct_write.cpp`, added to `REGRESSION_SUITES`.
- [ ] Memory receipt: all three inputs ≤ 320 MiB, lossless = 1. strace receipt as §5.
- [ ] `html-docs/guide.html` #saving lines 605, 609 and 615 updated for restore-on-failure, the "may be incomplete" message, and `sessions/unsaved.recovered/`; that folder also listed in #recovery beside `unsaved.corrupt/` and in the #config file-locations table (line 767) beside `unsaved.corrupt/` (R1-5, Q2b, R2-3).

**Task P1.11-B — F2 + F3 + F4 (draft cache and quit).** Branch `fix/p111-drafts`. Tests in `tests/regression/test_t1_11b_drafts.cpp`, added to `REGRESSION_SUITES`.
DoD:
- [ ] Placeholders first; cases 5–9 (incl. 5b, 6b) fail on them where they test new behaviour (record output; 5b and 6b may pass on the placeholders — state which).
- [ ] §3.2–§3.4 implemented exactly.
- [ ] `html-docs/guide.html` #recovery: the quit prompt when changes cannot be stored; the periodic-failure notification; line 700 extended to drafts whose file opens read-only (R1-5).

Both tasks:
- [ ] No added/modified function over 50 lines; no debug output; conventional commits with the attribution trailer.
- [ ] Verification commands (paste real output):
```
cmake -S . -B <build> -DCMAKE_BUILD_TYPE=Release -DOPENGL_INCLUDE_DIR=/usr/include -DPRIMEEDIT_BUILD_TESTS=ON
cmake --build <build> -j$(nproc) && (cd <build> && ctest --output-on-failure)       # all suites pass
cmake -S . -B <asan> -DCMAKE_BUILD_TYPE=Debug -DOPENGL_INCLUDE_DIR=/usr/include -DPRIMEEDIT_BUILD_TESTS=ON -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
cmake --build <asan> -j$(nproc) && (cd <asan> && ctest --output-on-failure)         # all pass, 0 sanitizer reports
python3 tools/check_function_length.py --base fdd5867
```

## 9. Open questions

1. **Logout with a full disk (F2).** Resolved (Umar Q1: accept). The prompt is the only moment the user can still act; a session-manager kill equals Quit and Discard and the previous on-disk set survives. Condition met by R1-2: no prompt when nothing is at risk.
2. **Recovery copy location (F1 step 5).** Resolved (Umar Q2: accept `sessions/unsaved.recovered/`). Conditions met: (a) path from `FileUtils::unsavedCacheBase()`, no `Document`→`Session` dependency (§3.1 step 5); (b) documented in guide #recovery (§8 task A DoD).

## 10. Round-1 findings index

| ID | Sev | Resolution |
|---|---|---|
| R1-1 | blocker | §3.1 step 3: Linux `fallocate` syscall, not `posix_fallocate`; step-3 failure truncates + `fstat`-checks before classifying; §5 strace receipt; DoD bans `posix_fallocate` |
| R1-2 | material | §3.2.1: false only when entries non-empty and replace failed; dialog says "changes not yet stored for recovery"; §5 case 6b |
| R1-3 | material | §3.5: overflow loop, BOM rule, ICU ignores `-BOM`; §5 case 10 ISO-2022-JP, GB18030, BOM-declared-without-BOM |
| R1-4 | material | §5 case 4b (Damaged on production path, packed exit report); case 4 uses pure `saveFailureMessage`, listed in §4 |
| R1-5 | material | §8 DoD: task A guide #saving 605/609/615 + recovered folder; task B #recovery 700, notification, quit prompt |
| R1-6 | material | header `fixes_artifact` added |
| R1-7 | minor | MiB throughout; BOM stripped via `QByteArrayView`; BOM input in memory receipt |
| R1-8 | minor | §3.2.2 restores exactly the timers active on entry; case 5 enables autosave, 5b without |
| R1-9 | minor | case 9 fixture corrected to `test_t1_3_encoding` line 140; draft pinned ASCII/UTF-8 |
| R1-10 | minor | §3.3: layer 1 kept, own reason stated |

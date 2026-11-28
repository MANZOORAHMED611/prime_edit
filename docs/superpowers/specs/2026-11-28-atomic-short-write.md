# Spec: atomic saves must detect a failed write or sync (P1.12)

**Status:** approved — Umar round 2 (scoped re-check). Round-2 notes N1, N2 applied after approval.
**Tier:** T2. Touches user files (every normal save) and stored data (draft cache, `session.json`, `settings.json`, named sessions, macros).
**Parent spec:** `2026-11-26-phase1-data-loss-fixes.md` (I1: a failed save never changes the file on disk)
**fixes_artifact:** escape from T1.1 (`39e093f`, atomic save path), T1.4 (`5f3a2ee`, `FileUtils::writeFileAtomic` for settings, session, named sessions, macros) and T1.5 (`0fce977`, draft cache via `writeFileAtomic`), all validated in `2026-11-27-phase1-output-gate.md`. Present in prime-edit 1.0.1 (`bfc7102`). Found by Uthman during P1.11-A; reproduced by the orchestrator and by Umar (RLIMIT_FSIZE and a real full tmpfs) on 2026-11-28.
**Base:** branch `phase1/release-1.0.1` after the P1.11 merge. "Fails before the fix" means fails on that base.

## 1. Objective

A Qt 6.2.4 `QSaveFile` opened with plain `QIODevice::WriteOnly` buffers writes up to its 16 KiB internal buffer. If the underlying `write(2)` fails — short (some bytes written, then ENOSPC/EFBIG/EDQUOT) **or writing nothing at all** (disk already full) — `QSaveFile::write()` still returns the full length and `commit()` returns **true**, so the truncated or empty temp file is renamed over the original.

Measured (Umar, from an existing 1 KiB file):
- `RLIMIT_FSIZE` 4096: buffered reports success and leaves a corrupt 4096-byte file for **every size 4097–16384**; 16385, 20000, 65536 fail correctly and keep the original. Unbuffered fails and keeps the original at all of them. 4000 saves correctly in both modes.
- Real ENOSPC (20 KiB tmpfs, 4096 B free): identical. Disk already full: buffered `commit()` returns true and leaves a **0-byte** file.
- Qt loops on short writes without error (write capped at 1000 B/call → full count, success), so unbuffered mode causes no false failures.

A second gap (R1-1): `QSaveFile::commit()` calls `fdatasync` but **ignores its result** in both modes (LD_PRELOAD shim forcing EIO: commit returned true, temp renamed over target). Errors that surface only at writeback (btrfs/CoW ENOSPC, quota, NFS, EIO) therefore also break I1.

Consequences today:
- **Normal save of a file up to 16 KiB on a nearly full or full disk:** the file is truncated or emptied, the editor shows it saved, the draft is cleared. Data lost.
- **Draft cache:** a truncated draft JSON is written "successfully", `replaceCacheSet` swaps it in and deletes the previous good set; at next start the draft fails to parse and is moved to `unsaved.corrupt/` — the text is not restored.
- **`session.json` / `settings.json` / named sessions / macros:** truncated JSON reported as written.

Target: for every payload size, any failed write (including one that writes nothing) or failed data sync makes both helpers report failure and leaves the destination byte-identical.

## 2. Scope

In: `Document::writeAtomic` and `FileUtils::writeFileAtomic` (the only two `QSaveFile` users in `src/`).

Out (logged to `docs/superpowers/backlog-phase2-3.md`):
- `ThemeManager` theme writes (`theme.cpp:692, 765`): truncate in place, result ignored.
- `LLMEvaluator::saveEndpoint` (`llmevaluator.cpp:132`): truncates the endpoint config (including the API key) in place, result ignored.
- Hex viewer Export Selection (`hexviewer.cpp:435`): reports success unconditionally. Out because it is an explicit export to a path the user chose, in a rare flow — not because it cannot overwrite (it can).
- `Document::saveRecovery` (`document.cpp:542`): live writer with **no reader** (`DocumentManager::saveRecoveryData` calls it on `m_recoveryTimer`; `recoverableDocuments`, `recoverDocuments`, `loadRecovery`, `hasRecoveryFile` have no callers). A truncated `.recovery` file loses nothing. Backlog entry: "live writer with no reader: removal candidate".

## 3. Design

1. Both helpers open the `QSaveFile` with `QIODevice::WriteOnly | QIODevice::Unbuffered`. Every `write()` then goes straight to `write(2)`, so a failed write returns a short count (or -1), which both helpers already treat as failure (`write(...) != size` → `cancelWriting()` → failure).
2. **Explicit data sync (R1-1, option a).** After a full `write()` and before `commit()`, both helpers call `::fdatasync(file.handle())`. If it fails: record the errno message (§4), `cancelWriting()`, return failure. `commit()`'s own (unchecked) sync stays; it is then a no-op cost.
3. Both helpers keep the rule "write returned the full size, the sync succeeded, and `commit()` succeeded"; no other logic changes.
4. The comment at `document.cpp:236-237` is rewritten to say: Qt's `commit()` calls `fdatasync` but does not check the result (Qt 6.2.4, verified with an LD_PRELOAD shim), so the helper syncs and checks itself.
5. Both helpers already pass the whole payload in one `write()` call, so unbuffered mode adds no extra syscalls (strace: a successful save is still a single `write`).
6. **`restoreOriginal` (post-P1.11, R1-7).** Its comment saying `writeFileAtomic` can commit a short write and still report success becomes false; rewrite it. The recovery-copy size check **stays** as defence in depth (harmless, one comparison).

## 4. Contracts

No signature changes. Behaviour:
- `Document::saveAs` returns `IoError` (not `Ok`) and `FileUtils::writeFileAtomic` returns `false` (not `true`) when the payload could not be written in full or the data sync failed, at every payload size.
- **Error message (R1-2).** After a short `write()` Qt sets no error (`errorString()` = "Unknown error", measured). `Document::writeAtomic` therefore captures `errno` immediately after a short `write()` or a failed `fdatasync` and sets `m_lastError = qt_error_string(errno)` (`errno` is cleared to 0 before the `write()` as a cheap safeguard, N2); it falls back to `file.errorString()` only if `errno` is 0. `writeFileAtomic` returns `bool` and needs no message.

## 5. Test strategy (failing tests first)

New suite `tests/regression/test_t1_12_short_write.cpp` in `REGRESSION_SUITES`. Forked child with `SIGXFSZ` ignored and `RLIMIT_FSIZE` set after all setup, as in `test_t1_1_save.cpp:44-68` (load before setting the limit).

1. **Document save sweep.** A 1 KiB original; limit 4096; new-content sizes {4097, 5000, 8192, 15000, 16383, 16384, 16385, 20000, 65536}: `save()` returns `IoError`, `isModified()` stays true, file bytes identical to the original, and `lastError()` is non-empty and not "Unknown error" (under RLIMIT it equals `strerror(EFBIG)`). **Plus a limit-0 run** (nothing can be written, R1-3) at size 8192 with the same assertions. (Fails today for 4097–16384 and for the limit-0 run.)
2. **writeFileAtomic sweep.** Same sizes and the same limit-0 run, existing 1 KiB destination: returns `false`, destination bytes identical.
3. **Draft cache end to end (R1-4).** In the parent, before `fork()`: build the `MainWindow`, one modified tab whose serialized draft (`QJsonDocument::toJson` output) is between **4097 and 16384 bytes**, and a previous good cache set; record its entry names and bytes. In the child under limit 4096: `Session::saveUnsavedDocuments` returns false. Afterwards: `sessions/unsaved/` holds **the same entry names with the same bytes** as the previous set, and every entry parses.
4. **Under-limit control.** Sizes ≤ 4000 still save `Ok` with correct bytes.
5. **Sync failure (R1-1).** Not reachable on ext4 by CTest. Receipt instead: an LD_PRELOAD shim making `fdatasync` return EIO (Umar's harness: `/tmp/claude-1000/-home-abusulaiman-dev-repository-prime-edit/5c051adb-108e-4ce7-b900-8902688defd5/scratchpad/umar-p112/shim.c`) run against a small driver for each helper: `IoError` / `false`, destination bytes identical, `lastError()` = `strerror(EIO)`. The shim source is committed under `tests/regression/tools/` or pasted into the receipt (the scratchpad path is not durable, N1). Alternatively a test seam, if the builder prefers; it is then listed in §4 and tested in CTest.

Existing suites must stay green unchanged.

**strace receipt:** one normal save under `strace -f -e trace=write,fdatasync,fsync,rename,renameat2,linkat` shows a single `write` of the payload, the helper's `fdatasync` on the temp fd, then the rename/link.

## 6. Blast radius

Every normal save and every settings/session/draft/macro write. The behaviour change is only on failure; the success path differs by skipping Qt's internal buffer copy and by one extra (already-cheap, data already synced) `fdatasync` call.

## 7. Rollback

Revert the merge commit. No on-disk format change.

## 8. Breakdown

**Task P1.12-A.** Owner **uthman**. Branch `fix/p112-short-write` from the post-P1.11 tip of `phase1/release-1.0.1`, own worktree.
DoD:
- [ ] Failing tests committed first; record the output (cases 1–3 fail at the buffered sizes and the limit-0 run).
- [ ] §3 implemented, incl. the explicit `fdatasync` and the §4 error message.
- [ ] `document.cpp` commit/fdatasync comment and the `restoreOriginal` comment rewritten (§3.4, §3.6); `restoreOriginal` size check kept.
- [ ] Backlog entries for the four out-of-scope writers (§2), with the reasons as worded there.
- [ ] No added/modified function over 50 lines; no debug output; conventional commits with the attribution trailer.
- [ ] Verification (paste real output): Release ctest all pass; ASan/UBSan ctest all pass with 0 reports; `python3 tools/check_function_length.py --base <base>`; strace receipt; sync-failure receipt (case 5).

## 9. Open questions

1. `Document::saveRecovery`: resolved (Umar R1-6) — out of P1.12; backlog as a removal candidate (§2).

## 10. Round-1 findings index

| ID | Sev | Resolution |
|---|---|---|
| R1-1 | material | §3.2 explicit checked `fdatasync` (option a); §3.4 comment; §5 case 5 shim receipt; §1 describes the gap |
| R1-2 | material | §4 error message from `errno` via `qt_error_string`; case 1 asserts not "Unknown error" |
| R1-3 | minor | §1 "including one that writes nothing"; limit-0 runs in cases 1–2 |
| R1-4 | minor | case 3 pins names+bytes, 4097–16384 serialized size, setup before fork |
| R1-5 | minor | §2 adds `llmevaluator.cpp:132`; hex export reason corrected |
| R1-6 | minor | §2/§9: `saveRecovery` live writer with no reader, removal candidate |
| R1-7 | minor | §3.6 `restoreOriginal` comment rewritten, size check kept; DoD item |
| R1-8 | minor | header adds T1.4 `5f3a2ee` |

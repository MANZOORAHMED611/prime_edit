# Spec: periodic session.json autosave (P1.10)

**Status:** round-2 re-check: revise (R1-2 open, R2-1, R2-2) — resolved inline, index in §10
**Tier:** T2 (touches stored data: `session.json`)
**Parent spec:** `2026-11-26-phase1-data-loss-fixes.md` (invariants I1–I5, D10 hold rule)
**fixes_artifact:** prime-edit 1.0.1 (`prime-edit_1.0.1_amd64.deb`, sha256 `84ae8a12…d1e3`, built from `bfc7102`), reproduced under `kill -9` on 2026-11-27.
**Base:** branch `phase1/release-1.0.1` at `64552c5` (= `bfc7102` + the inline-code preview fix). "Fails before the fix" means fails on `64552c5`.

## 1. Objective

After a crash, kill, power loss or logout-without-quit, PrimeEdit 1.0.1 restores
unsaved drafts (they are cached on a 30 s timer and on edits) but **not the list of
saved files that were open**, because `session.json` is written only in
`MainWindow::closeEvent`. Found by running the installed 1.0.1 under `kill -9`
on 2026-11-27: the draft came back, the two open files did not.

Target: after an abnormal exit, the reopened file list is at most
**5 seconds** stale. Today it is "as of the last clean quit" (unbounded).

## 2. Scope

In: periodic writes of the main `session.json` (file list, bookmarks, folds,
active tab, geometry) while the window is running.
Out: named sessions (`sessions/<name>.json`, explicit user action, unchanged);
the draft cache (`sessions/unsaved/`, already periodic, unchanged); the
"restore previous session" preference semantics (unchanged: it controls
*reading*, not writing — today's quit path writes regardless).

## 3. Design

1. **Skip-if-unchanged, failure-safe (R1-3).** `Session::save(MainWindow*)`
   keeps `m_lastWritten`, the JSON of the last *successful* write. Order:
   (a) D10 hold check (below) first, unchanged in position; (b) if the new
   JSON equals `m_lastWritten`, return without touching disk; (c) call
   `FileUtils::writeFileAtomic`; update `m_lastWritten` **only when it returns
   true**. A failed write therefore leaves the next tick and the quit-time
   save free to retry.
2. **D10 hold baseline (R1-1).** The hold compares a *user-state* projection of
   the session — `files` (paths, bookmarks, folds) and `activeTab` — and
   **excludes `geometry`**, because window-manager placement after `show()`
   changes x/y with no user action. The baseline `m_heldSession` is re-taken
   when autosave is enabled, i.e. after `restoreAndOpen()` has opened the
   command-line files and closed the empty Untitled tab, so startup itself is
   never read as a user change. `Session::rebaseHold(MainWindow*)` does this
   and is a no-op when no hold is active. Once the user changes the file
   list, a bookmark, a fold or the active tab, the hold is released and the
   full JSON (geometry included) is written.
   **Declared change to D10 (R2-1, option a):** a window move or resize alone
   no longer releases the hold. Window placement is not user content, and
   counting it would let window-manager drift overwrite the corrupt original
   seconds after every start. Consequences, both in scope:
   - `tests/regression/test_t1_4_persistence.cpp:172-177`
     (`truncatedSessionKeptAndNotOverwritten`) is amended: it first asserts
     that a resize alone followed by `save()` leaves the corrupt file
     byte-identical, then releases the hold by toggling a bookmark and
     expects the valid file (with the new width) as before.
   - `html-docs/guide.html:707` and `html-docs/faq.html:536` change "until you
     change something" to "until you open or close a file, or change a
     bookmark, a fold or the active tab".
3. `MainWindow` gets `m_sessionAutosaveTimer` (QTimer, repeating), **not
   started in the constructor**. Interval 5000 ms; a static
   `MainWindow::setSessionAutosaveIntervalForTest(int ms)` overrides it for
   unit tests (precedent: `Document::setSizeThresholdsForTest`) (R1-8).
4. `MainWindow::setSessionAutosaveEnabled(bool)` starts/stops the timer;
   enabling also calls `Session::rebaseHold(this)`. `main.cpp` calls
   `setSessionAutosaveEnabled(true)` **after** `restoreAndOpen()` returns.
   This ordering is **load-bearing (R1-7)**: `openFile` shows a modal
   `QMessageBox::warning` for an unopenable file, which runs a nested event
   loop; a timer already running there would write a half-restored list.
5. On timeout: `Session::instance().save(this)`.
6. `closeEvent` stops the timer **first**, before the existing
   `saveUnsavedDocuments` + `save` calls, so no tick can run while editors
   are being torn down.

**Accepted transient (R1-7).** A tick that fires inside a modal dialog after
startup (Close All save prompts in `confirmCloseEditor`, a named-session
load) writes the live, partially changed window state. That is the true state
at that instant; the next tick (≤ 5 s) writes the settled state. Accepted:
the worst case after a crash in that window is reopening a file the user was
in the middle of closing.

Why a periodic timer instead of hooking open/close/move/fold/bookmark signals:
the session contains state changed from many places (bookmarks, folds, pane
moves, geometry); a 5 s poll with an equality check covers all of them with
one code path and zero disk writes when idle. Cost per tick: building a JSON
object over the open editors (microseconds for tens of tabs) and one compare.

## 4. Contracts

- `void MainWindow::setSessionAutosaveEnabled(bool enabled);` — public; enabling rebases the D10 hold.
- `bool MainWindow::sessionAutosaveActive() const;` — public, for tests.
- `static void MainWindow::setSessionAutosaveIntervalForTest(int ms);` — tests only.
- `void Session::rebaseHold(MainWindow *window);` — public; no-op without an active hold.
- `Session::save()` — signature unchanged. No-op when the JSON equals the last
  successful write; `m_lastWritten` changes only when `writeFileAtomic` returns true;
  the hold check runs first and compares the geometry-free projection.
- File format of `session.json`: **unchanged** (1.0.0 and 1.0.1 read it as before).
- Write mechanism: unchanged (`FileUtils::writeFileAtomic`, QSaveFile).

## 5. Test strategy (failing tests first)

The two new `MainWindow` members and `Session::rebaseHold` land first as
**1.0.1-behaviour placeholders** (setter stores the flag, starts nothing;
`rebaseHold` does nothing) so the suite compiles and the failing cases fail at
runtime, not at compile time (R1-6, parent R2-1).

New suite `tests/regression/test_t1_10_session_autosave.cpp`, added to
`REGRESSION_SUITES` (isolated HOME). Unit cases use
`setSessionAutosaveIntervalForTest(200)`.

**Fail on base:**
1. `openFileIsPersistedWithoutQuit` — enable, open a file, QTRY ≤ 1 s:
   `session.json` lists it; no close.
2. `closedFileIsDroppedWithoutQuit` — open two, close one, list shrinks.
3. `failedWriteIsRetried` (R1-3) — open a file; make the AppData dir
   unwritable (chmod 0500) **before** enabling; enable; wait > 2 intervals
   and assert `session.json` is absent; restore 0700; with **no state
   change**, QTRY: `session.json` lists the file.
4. `corruptHoldSurvivesRealStartup` (R1-1) — write a corrupt `session.json`
   and one cached draft; build the window the way `main.cpp` does
   (`restoreDrafts`, `restoreFiles`, open a file argument, close the empty
   Untitled tab, `show()`, then enable); wait > 3 intervals with no user
   action: the corrupt original is byte-identical. Then toggle a bookmark:
   QTRY a valid `session.json` is written. (Fails on base only in its second
   half; the first half is a guard.)
5. `processRecordsFilesBeforeCrash` (R1-2) — process level, on
   `$PRIMEEDIT_BIN` with the T1.6 harness (scratch profile, short runtime
   dir, offscreen): launch with a file argument; within **10 s**
   `session.json` lists it; SIGKILL; record the inode of `session.json`;
   relaunch with no arguments (restore preference default on); within
   **10 s** the inode changes (QSaveFile renames, so every write is a new
   inode, and a fresh process has an empty `m_lastWritten`, so its first
   tick always writes) and the new file lists the file — proving the
   relaunched process reopened it. Nothing is deleted. This is the §1
   reproduction and the only case that catches a missing `main.cpp` call.

**Guards (pass on base, must stay green):**
6. `noWriteBeforeEnabled` — open a file, never enable, 3 intervals: no file.
7. `unchangedStateDoesNotRewrite` — after one write, 3 ticks with no change:
   inode and mtime unchanged.
8. `closeStopsAutosave` — after `close()`, `sessionAutosaveActive()` is false
   (state assertion only; the teardown race cannot be reproduced in-process).

Plus the existing 19 suites (with the declared `test_t1_4` amendment, §3.2)
and the ASan job stay green; expected added
runtime ≈ 15 s, dominated by case 5.

## 6. Blast radius

`src/core/session.{h,cpp}` (save, rebaseHold, two members),
`src/ui/mainwindow.{h,cpp}` (timer, setters, closeEvent first line),
`src/main.cpp` (one call), one new test file + CMake entry, the declared
amendment to `tests/regression/test_t1_4_persistence.cpp` (§3.2), and the user docs
`html-docs/guide.html` (#recovery) and `html-docs/faq.html` (unsaved-work
answer) which today describe crash recovery as drafts-only (R1-4). No format
change, no new dependency.
Behavioural change visible to users: open files return after a crash.
Disk: at most one small atomic write per 5 s, only while state changes.

## 7. Rollback

Revert the commit; `session.json` format is unchanged, so a file written by
the autosave build is read identically by the previous build. No data
migration either way.

## 8. Breakdown (R1-4)

**Task P1.10-A — session autosave.** Owner: **uthman** (persistence layer).
Base: `phase1/release-1.0.1` @ `64552c5`; work on branch `fix/session-autosave`
in its own worktree; no dependencies.

DoD:
- [ ] Placeholders committed first; cases 1–5 fail on them (record output).
- [ ] §3 implemented exactly; `closeEvent` stops the timer on its first line.
- [ ] `test_t1_10_session_autosave.cpp` with all 8 cases, in `REGRESSION_SUITES`.
- [ ] `html-docs/guide.html` #recovery and `html-docs/faq.html` updated: open files are recorded within 5 s, so after a crash they reopen (with the restore preference on); drafts unchanged.
- [ ] `test_t1_4_persistence.cpp` amended exactly as §3.2 declares; guide/faq hold wording corrected.
- [ ] No added/modified function over 50 lines; no debug output; conventional commits with the attribution trailer.

Verification commands (paste real output in the receipt):
```
cmake -S . -B <build> -DCMAKE_BUILD_TYPE=Release -DOPENGL_INCLUDE_DIR=/usr/include -DPRIMEEDIT_BUILD_TESTS=ON
cmake --build <build> -j$(nproc)
(cd <build> && ctest --output-on-failure)            # 20/20
cmake -S . -B <asan> -DCMAKE_BUILD_TYPE=Debug -DOPENGL_INCLUDE_DIR=/usr/include -DPRIMEEDIT_BUILD_TESTS=ON -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
cmake --build <asan> -j$(nproc) && (cd <asan> && ctest --output-on-failure)   # 20/20, 0 sanitizer reports (CI job "asan")
python3 tools/check_function_length.py --base 64552c5
```

## 9. Open questions

- Interval: 5 s chosen (bounded staleness vs. churn while dragging the window;
  the equality check makes idle cost zero). Geometry changes during a drag
  could write at most once per 5 s.

## 10. Round-1 findings index

| ID | Sev | Resolution |
|---|---|---|
| R1-1 | material | §3.2 geometry-free hold projection, rebaseline at enable; §5 case 4 follows real startup order |
| R1-2 | material | §5 case 5, process-level reproduction on `$PRIMEEDIT_BIN` |
| R1-3 | material | §3.1 `m_lastWritten` only on successful write; §5 case 3 |
| R1-4 | material | §8 breakdown with owner, base, DoD, commands; docs in §6 |
| R1-5 | minor | header `fixes_artifact` and `Base` |
| R1-6 | minor | §5 placeholders; guards labelled; case 8 reduced to its state assertion |
| R1-7 | minor | §3.4 ordering is load-bearing; accepted modal transient stated |
| R1-8 | minor | §3.3 interval test hook; runtime stated in §5 |
| R1-2 (r2) | material | §5 case 5: inode-change observation, no deletion, 10 s windows |
| R2-1 | material | §3.2 option (a) declared: resize alone keeps the hold; `test_t1_4` amendment and doc wording in scope (§6, §8) |
| R2-2 | minor | §5 case 3: chmod before enable; assert absent while unwritable |

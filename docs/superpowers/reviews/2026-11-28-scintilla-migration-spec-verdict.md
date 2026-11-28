# Spec verdict: Scintilla + Lexilla editor-core migration

**Spec:** `docs/superpowers/specs/2026-11-28-scintilla-migration.md` (commit 31eae31, sha256 `f9bdff7efe5ac41dffc9ee989d306ec6fe46f5741a2332fb4a8fabca70527ad8`)
**Tier:** T3
**Author:** Abu Bakr
**Reviewer:** Umar (fresh spawn, not the author)
**Transcribed by:** the orchestrator, from Umar's returned text

## Round 1

**Opened:** 2026-11-28T13:20:00Z
**Verdict at:** 2026-11-28T14:15:11Z
**Verdict: revise.** No blockers; 14 material findings, 5 minor.

### Summary

The core design holds up:
- The save path really is limited to the byte source.
- The fast-path guard is correct for UTF-8, BOM files, mixed EOL and the direct-write path.
- The one-pass and legacy branches match `normalizeLineEndings` + `TextCodec::encode`.
- I2 (save point OR formatDirty OR draftDirty) is sound.
- All 47 Lexilla lexer names exist in 5.5.5, and the 96 language ids match `resources/syntax` exactly.

The 14 material findings keep this from approval; most need a paragraph or a table row each. Round 2 is a scoped re-check of the fixes to R1-1 to R1-14 only.

### Rubric

| # | Item | Result | Note |
|---|---|---|---|
| 1 | Direction | concern | invariants table incomplete (R1-3); npp-qw1 features not mapped (R1-4) |
| 2 | Simplicity | concern | the KeyChordDispatcher premise is contradicted by a probe (R1-12); qt6-wayland is not needed (R1-13) |
| 3 | Contracts | concern | B2 lifetime (R1-1), B1 entry paths (R1-2), memory targets (R1-7), L criterion (R1-8), LargeView O(n) positions (R1-9) |
| 4 | Test strategy | concern | assertion guard gaps (R1-10); timing asserts under ASan (R1-11); chord question answerable now (R1-12) |
| 5 | Blast radius / rollback | concern | rollback concrete; master-drift policy missing (R1-14); plugin API change unstated (R1-15) |
| 6 | Breakdown | concern | SB2 cannot be green before SB3 (R1-5); ownership overlaps and an SA1/SA2 cycle (R1-6) |
| 7 | Tier / fixes_artifact | pass | T3 correct; §2.3 corrects the spike's diagnosis |
| 8 | Domain | pass | irreversible steps stay user-confirmed; the bidi regression is the owner's ruling |

### Findings

| ID | Sev | Location | Issue | Required change |
|---|---|---|---|---|
| R1-1 | material | §3.3 B2; §3.4 step 5 | Synchronous slots (`finishSave` emits `modifiedChanged` before hashing `written`; `setSavePoint` signals) run while the zero-copy view is live. The read-only guard contradicts §3.2 (programmatic edits lift read-only), so the pointer can dangle during the hash. | Compute SHA-1 and size from the view, then drop it before any signal or `setSavePoint`. `finishSave(path, sha1, size)`. Replace the read-only guard with a debug assert on a DocWatcher change counter. S1 case 6: a slot on `modifiedChanged` calling `setText` must leave the file and the emitted hash equal to the pre-save buffer. |
| R1-2 | material | §3.3 B1; §3.5.1 step 3; §3.5.2 Preview | Valid-UTF-8 invariant has two unspecified entry paths: lenient detection or forced Reopen-as-UTF-8 of invalid bytes; Preview can split a code point at 2 MiB. | A failed strict validation falls through to step 4 (lossy decode, read-only), never raw bytes. Preview truncates at the last complete code point (UTF-8 never before a continuation byte; UTF-16 even count, no split surrogate). Two S1 cases. |
| R1-3 | material | §3.3 invariants table | Missing rows: D1 (`editor.cpp:97` SyncTarget::Modified on bind → draftDirty on Move to Other View), D4/D6 restore order, D8 (own-save hash from view; reload re-attach), D9 (no-break default in byte-level `normaliseEols`), P1.11 F2–F4 (F4 now also routes LargeView/LongLineView drafts). | Add rows with mechanism and proof suite. State whether the Phase B Editor constructor applies any SyncTarget (attach-only preferred). Empty and no-break files in S1 case 4. A restore case whose draft target opens as a LongLineView and lands in an untitled tab. |
| R1-4 | material | §3.7.1; §4.2; header line 9 | npp-qw1 Editor members are unmapped: `goToLineColumn`, `insertBlankLine`, `setBookmarks`, `selectWordIfNoSelection`, `selectBetweenMatchingBraces`, `markAll(...,from,to)`, `commentSyntax()` (which goes through the SyntaxHighlighter that SB3 deletes). `mainwindow_npp_*` use `textCursor()`. Citations are at 4d35896 although npp-qw1 rewrites mainwindow*. | After npp-qw1 merges, regenerate §3.7.1 and §4.2 against that commit, add the members with mechanism and owner, and re-cite. Name the post-SB3 source of `commentSyntax()` and give it to a single task. |
| R1-5 | material | §7.2 step 2; §10.3 SB2 DoD | SB2 cannot be green without SB3: `test_t2_3_folding` needs lexer fold levels; `test_npp_qw1:517-522` asserts `commentSyntax()` via the highlighter. | (a) Move a minimal `LexerCatalog::apply` (fold=1, no theme) and the `commentSyntax` source into SB2, or (b) merge SB2 and SB3 as one step. Update the DoD. |
| R1-6 | material | §10.1–§10.3 | `lspbridge.cpp` is owned by SA2 but moved in SA1's DoD. SA1/SA2 dependency cycle (SA1 needs SA2's `replaceContent`). `guide.html` is owned by SB5 but SB3 updates `#themes`. | `lspbridge` goes to SA2 only. Order: SA2 step 1 (`replaceContent`), then SA1, then the rest of SA2. Move `#themes` to SB5, or SB3 owns that anchor and merges before SB5. |
| R1-7 | material | §1; §3.5.1–§3.5.2; §5.6; §7.2 | Memory model mismatch: the cited figures are VmHWM peaks (settled 637,044 / 1,125,408 kB); the spike streamed 16 MB chunks with no whole-file array, while §3.5.1 reads the whole file (+1.0x); kB is KiB, so the factors are 1.39/2.40; PrimeEdit's baseline is about 160 MB; targets have no unit or basis; no target gates the cut-over; SB6 omits the legacy and UTF-16 factors. | (1) Stream UTF-8 loads in 16 MiB chunks, or restate the factors with the array included. (2) Absolute RSS targets in MiB/GiB with a same-build baseline. (3) Name the gating rows; a miss over 15% needs a spec amendment before merge. (4) SB6 also measures 100 MiB windows-1256 and UTF-16LE files. |
| R1-8 | material | §3.5.3; §10.3 SB6 | The L sweep (1–8 MiB) cannot lower L (cap 1 MiB); first paint is the wrong metric for an editable tier. | Sweep 64 KiB, 256 KiB, 512 KiB and 1 MiB; measure first paint and keystroke-to-paint at line end (median and max of 20). L = the largest value with first paint ≤ 500 ms and keystroke ≤ 50 ms. Record it in the receipt. |
| R1-9 | material | §4.2; §3.8.8; §5.5 | `StatusBarWidget::updateFromEditor` (`statusbar.cpp:139-141`) on each caret move needs an O(file) UTF-16 position without a line index on LargeView; Select All builds a ~1 GB QString; int positions overflow past 2^31 UTF-16 units. | Line-character index for every tier (measure the cost), or line/column semantics for LargeFile. Selection length via `SCI_COUNTCODEUNITS`. Cap the view so the UTF-16 length fits in int (else Preview), or use qint64. Two `test_s4_views` cases: caret-move status time after Ctrl+End; Select All allocates < 1 MiB. |
| R1-10 | material | §5.2 rule 2; §10 | The assertion guard misses `QTRY_*` (6 uses), added `QSKIP`/`QEXPECT_FAIL`, and deleted test functions. | `grep -E '^[-+].*\b(QCOMPARE\|QVERIFY2?\|QTRY_[A-Z_]+\|QSKIP\|QEXPECT_FAIL\|QFAIL)\b'`; no added QSKIP or QEXPECT_FAIL; each suite's `-functions` listing identical before and after, pasted in the receipt. |
| R1-11 | material | §5.6; §5.2 rule 4; §3.5.3 9a/9b | 2x timing asserts will fail or flake under the -O0 ASan gate. | Timing asserts only in non-sanitizer Release builds (a CMake definition when -fsanitize is absent); under ASan, print without asserting. |
| R1-12 | material | §3.1; §3.7.1 row 32; §5.1; SB2 DoD | The KeyChordDispatcher premise is an untested code comment. Umar's probe (Qt 6.2.4, offscreen) shows the global modifiers are set for both `QTest::keyClick(Ctrl+End)` and a non-spontaneous `sendEvent` (the MacroRecorder path). | Extend `test_s0_probe` to a real ScintillaEdit with both cases. Build the dispatcher only if the sendEvent case fails; otherwise delete it and correct §5.1. |
| R1-13 | material | §3.9; §6; §9 Q2; SB6 | `Recommends: qt6-wayland` has no felt failure here and moves the whole app to native Wayland (IME verified only on nested Weston; the D4 single-instance raise needs an activation token). "As directed" has no source. | qt6-wayland is not added in this programme (or Suggests). Remove it from §3.9, BUILD.md and SB6. Native Wayland becomes its own item, with a raise test. |
| R1-14 | material | §7.2; §10.4 | No master policy for the ~10-week Phase B window. | Freeze `[E]` work and changes to `src/ui/editor*`, `src/core/document*` and `src/syntax/*` on master; editor-agnostic work may merge. Merge master into `migration/scintilla` at each SB merge and re-run Release + ASan; the file's Phase B owner resolves conflicts. |
| R1-15 | minor | §3.8.7; §3.7.2 C1/C2 | `EditorAPI::currentText/selectedText` (`mainwindow.cpp:2581-2607`) change for plugins: NBSP/U+2028 come back raw, and LF replaces U+2029. Public interface. | State it in §3.8.7 and in the RELEASE_NOTES (SB6). |
| R1-16 | minor | §3.5.3; §3.3 I5 | L is enforced only at load; long lines can enter through draft restore, paste or join. | State the limit; SB6 measures restore of a draft with a 5 MiB line; record it as a known limit or R5. |
| R1-17 | minor | §3.2; §4.1 | (a) `replaceContent` must restore the save point; (b) `setText` becoming an undoable action changes the undo-to-clean semantics; (c) SyncTarget::Preserve loses its only caller. | State all three. |
| R1-18 | minor | §3.3 B3; §3.5.1 step 3 | T1.8 legacy text can carry `\r` (harmless, read-only, caught by the guard); the NUL-beside-break check must run before normalisation. | Narrow B3 to "except T1.8 read-only documents"; order: validate, NUL check, normalise. |
| R1-19 | minor | §3.2; §3.7.2; §9 Q1; §1 | (a) `test_t1_1_save.cpp:196-210` calls setText before setReadOnly; (b) nine test files plus `sync_failure_driver.cpp` call setText, not seven; (c) the Medium tier with lines from 8,000 chars to L becomes editable (unlisted change); (d) R2–R4 name no one who accepted them; (e) the Notepad++ freeze claim is unverified; (f) project targets not cited. | Correct (a) and (b); list (c); mark R2–R4 "proposed"; mark (e) [UNVERIFIED] or drop it; add the CLAUDE.md targets to §1. |

### Open questions

- **Q1:** stands, provided L is set by R1-8 and R1-19e is fixed.
- **Q2:** resolved without the owner: no qt6-wayland in this programme (R1-13).
- **Q3, Q4, Q5:** stand.
- **Owner needed:** none.

### Evidence checked
- the spike report and evidence files; `demo/bench.cpp:69-81`; `demo/selftest.cpp:56-58`
- the Phase-1 spec (I1–I5, D1–D12) and the P1.11 spec
- `document.cpp`, `charsetdetector.cpp`, `session.cpp`, `editor.cpp`, `mainwindow.cpp`, `statusbar.cpp` and `macrorecorder.cpp` at 4d35896
- `origin/feature/npp-qw1` at ce5d206
- Lexilla 5.5.5 module names (133; all 47 present)
- assertion macro counts
- Probe: `scratchpad/keyprobe/probe2.cpp`, output `keyClick(Ctrl+End), keyClick(End): hits=2 global_ctrl=1 after=0` and `sendEvent only: hits=1 global_ctrl=1`

### Next step
The author applies R1-1 to R1-14 and dispositions R1-15 to R1-19 in the same commit. Round 2 is a scoped re-check of the R1-1 to R1-14 fixes only; if it does not reach approve, the matter goes to Mohammed with both positions. Phase A stays held until npp-qw1 merges and R1-4 is regenerated against that commit.

# Output gate — Phase 1 data-loss fixes (T3)

gate_agent: umar · gate_result: **validated** · tip: `ad2ca68` (phase1/base) · date: 2026-11-27

## Evidence
- Release ctest at the tip: 19/19. ASan/UBSan ctest at the tip: 19/19, sanitizer count 0.
- Fail-before: all 20 task suites, copied onto the W0 base `6fcb61e`, fail on their target defect. T3.2 was also checked at its parent `85e59e4`.
- `tools/check_function_length.py --base 6fcb61e`: OK. Every touched legacy function shrank or held its length; for example, `Document::load` went from 143 to 119 lines.
- No qDebug, printf or cout added in `src/`.
- Docs: no "clone to other view" left; every §6 behaviour is documented, and the UI strings quoted in the docs match the code.
- Contracts: every one in §4 and §9.1 is present in producer and consumer.
- Integration check (Abu Bakr): pass. Its one material finding, M1 (duplicate drafts for one path collapse), is fixed as T1.9.

## Deviations, all accepted
- a. **Length grandfathering.** Legacy functions over 50 lines may be edited if they don't grow. Logged in the backlog.
- b. Fail-before trees built with `git archive` instead of `git worktree add`, because the sandbox refused worktrees.
- c. Diffs read instead of piping them through grep. The gate then ran the grep itself and found nothing.
- d. T1.4 turned off LeakSanitizer in the forked test child only.
- e. T1.6 uses a short scratch runtime dir, because of the 108-byte socket path limit.
- f. **Auto-save timer.** Hiding the preference does not stop it. `startRecoveryTimer()` always runs and writes `recovery/*.recovery` files that nothing reads. This is unchanged 1.0.0 behaviour; removing it is in the backlog.
- g. When the socket path is too long, the second launch exits with "not responding". This fails safe: it never starts a second writer.

## Minor findings
- m1–m2. W3 scope drift: the D11 `rawText()` switch was extended to line commands and Replace All. Beneficial and tested.
- m3–m4. W3 edited `html-docs/index.html` and `README.md` beyond its assignment. Accurate.
- m5. Two T1.6 cases have a weak base discriminator. The defect itself was confirmed on disk.
- m6. T3.2 and T3.3 have precondition failures at the W0 base. T3.2 was confirmed at its parent.
- m7. T0.1 made a one-line CMake change in T0.2's file, as its own breakdown required.
- m8. The T1.8 defect escaped T1.3's suite and was caught by T4.1.
- m9. No receipts were kept on disk. They are transcribed below.

## Builder scores
| Builder | Tasks | Weighted |
|---|---|---|
| uthman | T0.1, T1.1–T1.8 | 8.66 |
| ali (W2) | T2.1–T2.4 | 10.0 |
| ali (W3) | T3.1–T3.5 | 8.70 |
| abdullah | T0.2, T4.1 | 10.0 |

## Receipts (condensed)
| Task | Commit | Fail-before | Result |
|---|---|---|---|
| T0.1 | 49dc4c7 | n/a | interfaces with placeholder bodies; behaviour identical to base |
| T0.2 | 6aec9c6 | n/a | ICU required; 16 suites; length tool; ASan CI job |
| T1.1 | 39e093f | 8/8 | QSaveFile save; refusals; direct write; hard link kept. QSaveFile fdatasyncs (strace) |
| T1.2 | a163f65 | 6/6 | format-dirty flag; LF default; mixed-ending record |
| T1.3 | b32d54b | 10/10 | ICU TextCodec; whole-file detection; lossy decode opens read-only; BOM kept. 45 MB load 560→660 ms, same RSS |
| T1.7 | b6b5221 | 4/4 | own save is not reported as an external change |
| T1.4 | 5f3a2ee | 4/4 | atomic persistence files; `.corrupt-*` copy; no write until a change |
| T1.5 | 0fce977 | 13/13 | cache recovery table; restore into the right editor; unplaced drafts kept; crash_test green |
| T1.6 | bf07eae | 5/5 | single instance per profile; IPC with ack; startup order |
| T1.8 | 62947fb | 5/5 | BOM-less UTF-16 detection; NUL safety net |
| T2.1 | 017a893 | 3/3 | RTL pass keeps flags; Latin blocks untouched |
| T2.2 | 4d1d10a | 3/3 | `rawText()` sync; flags left to the save result |
| T2.4 | 8e2dd6a | 3/3 | read-only mirrored; key guard; no banner text in the buffer |
| T2.3 | ba09b89 | 11/11 | view-only folding; unfold rules; Go to Line by block |
| T3.1 | 166cc69 | 4/4 | Clone removed; close routed by sender; lifetime guard |
| T3.2 | c6aed67 | 5/5 | save by editor; SaveResult UI; partial-view banner; close prompt on `isModified` |
| T3.3 | f8d9322 | 10/10 | convert preflight; Reopen with Encoding; EncodeLossy dialog; Replace All keeps U+00A0 |
| T3.4 | 863cb31 | 16/16 | two-action reload bar; background tabs; line commands use `rawText()`; read-only conversions refused |
| T3.5 | a52c8e6 | grep | docs, README, Auto-save preference hidden |
| T4.1 | 6690901 | 62/98 | I1 matrix, 98 cases; found T1.8 |

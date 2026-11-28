# Spec verdict: PR #5 review findings (P1.11)

Spec: `docs/superpowers/specs/2026-11-27-pr5-review-data-safety.md`

## Round 1 — Umar — verdict: revise

- spec_sha256 (round 1 input): `2edc3cef3e29c730c9eea945af8cd17dbbbdb746cf013236625f9b9e2d6e5513`
- verdict_at: 2026-11-28T03:45:00Z · tier: T2
- Rubric: 1 Direction pass · 2 Simplicity concern (R1-10) · 3 Contracts **blocker** (R1-1, R1-2, R1-3) · 4 Test strategy concern (R1-3, R1-4) · 5 Blast radius/rollback pass · 6 Breakdown concern (R1-5) · 7 Tier/fixes_artifact concern (R1-6) · 8 Domain pass
- Independently confirmed every defect F1–F5 at fdd5867; re-ran the F5 probe (404 / 407 MiB); verified case 1 (fallocate → EFBIG under RLIMIT_FSIZE on ext4, st_size unchanged), case 7 trigger reachable (directory → QFile::open fails → modal at mainwindow.cpp:819), AllowDirect rerouting keeps test_t1_1 / test_t3_2 green; chunked encoder equivalence measured for Qt UTF codecs and ICU (ISO-2022-JP/KR/CN, HZ, IBM930, Shift_JIS, EUC-JP, GB18030, UTF-7, windows-1252) provided U_BUFFER_OVERFLOW_ERROR is looped.

Findings:

- **R1-1 (blocker, §3.1 step 3).** glibc `posix_fallocate` never returns EOPNOTSUPP; on filesystems without fallocate it emulates by writing a NUL byte per block past EOF (reproduced with `strace -e inject=fallocate:error=EOPNOTSUPP`: file left at 16384 bytes, EFBIG). Step 3 would then report Unchanged over a NUL-padded file. Required: Linux `fallocate(fd,0,0,n)` syscall; step-3 failure runs `ftruncate(original.size())` + `fstat` check before classifying; strace receipt.
- **R1-2 (material, §3.2).** `replaceCacheSet` fails even for an empty list on a full/unwritable disk, so quit would always prompt with nothing to lose; dialog overstates the loss. Required: false only when entries non-empty and replace failed; reword; quiet-quit case.
- **R1-3 (material, §3.5 / case 10).** ICU slice loop must handle U_BUFFER_OVERFLOW_ERROR (per slice and flush); windows-1252 exercises no state; BOM rule unpinned. Required: state overflow loop, BOM rule, ICU ignores -BOM; case 10 adds ISO-2022-JP straddle ending in JIS mode, GB18030 4-byte split, BOM-declared-without-BOM.
- **R1-4 (material, case 4).** Damaged classification tested only via an unspecified seam; reachable on the production path (8 KiB original, 6 KiB new, limit 4096). Required: that case; seams listed in §4.
- **R1-5 (material, §8).** Docs DoD misses guide.html #saving 605/609/615 (task A) and #recovery 700 + tick notification (task B).
- **R1-6 (material, header).** `fixes_artifact` missing; these are escapes from validated T1.1 39e093f, T1.3 b32d54b, T1.5 0fce977, T1.9.
- **R1-7 (minor).** Probe reports MiB; BOM path copies via `bytes.mid()` — use QByteArrayView, add BOM input to receipt.
- **R1-8 (minor).** Cancel restarts timers unconditionally; restore the on-entry state; case 5 enables autosave, add variant.
- **R1-9 (minor).** Lossy fixture is test_t1_3_encoding.cpp:140; pin draft ASCII/UTF-8.
- **R1-10 (minor).** Layer 2 alone closes F3; state layer 1's own reason or drop it.

Open questions: Q1 accept (condition: no prompt when nothing at risk). Q2 accept `sessions/unsaved.recovered/` (conditions: shared path helper, no Document→Session dependency; document the folder).

spec_verdict: docs/superpowers/specs/2026-11-27-pr5-review-data-safety.md — Umar round 1, verdict: revise

Orchestrator: all ten findings applied inline; resolution index in spec §10.

## Round 2 — Umar — scoped re-check — verdict: approve

- spec_sha256 (round 2 input): `972c8e9d25f9f2a6da6fe503279873eac005a4ca15e916b83a2a81c331b8468e` · base `fdd5867` · tier T2
- Scope: fixes to R1-1..R1-6 only.
- Independently verified: glibc `fallocate()` is a direct syscall (driver under `strace -e inject=fallocate:error=EOPNOTSUPP`: returns -1 EOPNOTSUPP, st_size unchanged, no NUL padding; uninjected on ext4 returns 0); today's BOM-declared-without-BOM is lossy (rule accurate); case 4b reaches Damaged with empty recovery path deterministically (hand-traced); guide.html lines 605/609/615/700 and #recovery span confirmed; header commits 39e093f, b32d54b, 0fce977, ec3eacd, bfc7102 exist.
- R1-1 closed · R1-2 closed · R1-3 closed · R1-4 closed · R1-5 closed · R1-6 closed (all in round 2). R1-7..R1-10 ride along as author notes.
- New minor: R2-1 §4 row missing "nothing to store" case; R2-2 strace receipt must pin Restored under injection (use a driver); R2-3 optional guide.html:767 file-locations row for `unsaved.recovered/`.

spec_verdict: docs/superpowers/specs/2026-11-27-pr5-review-data-safety.md — Umar round 2, verdict: approve

Orchestrator: R2-1, R2-2, R2-3 applied to the spec after approval (notes, no re-submit per governance).

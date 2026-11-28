# Spec verdict: atomic saves must detect a failed write or sync (P1.12)

Spec: `docs/superpowers/specs/2026-11-28-atomic-short-write.md`

## Round 1 — Umar — verdict: revise

- spec_sha256 (round 1 input): `05a5b264c64b0f74a029a7c089f37e534213639e631279e06d4b7484b79e19ff` · verdict_at 2026-11-28T04:12:16Z · tier T2
- Rubric: 1 pass · 2 pass · 3 concern (R1-1, R1-2) · 4 concern (R1-3, R1-4) · 5 pass · 6 pass · 7 concern (R1-8) · 8 pass
- Measured (harness `scratchpad/umar-p112/`): buffered QSaveFile corrupts every size 4097–16384 under RLIMIT_FSIZE 4096, unbuffered keeps the original at all sizes; Qt loops on short writes without error (no false failures); real ENOSPC on a user-namespace tmpfs behaves the same; a full disk gives a 0-byte file with buffered mode; commit() calls fdatasync before rename in unbuffered mode but ignores its result (LD_PRELOAD EIO shim).

Findings:
- **R1-1 (material).** commit() ignores a failed fdatasync in both modes; I1 still breaks for writeback-time errors. Required: explicit checked fdatasync before commit (option a) or declare out with backlog; fix the document.cpp comment.
- **R1-2 (material).** After a short write Qt's errorString() is "Unknown error". Required: message from errno via qt_error_string; case 1 asserts it.
- R1-3 (minor) zero-byte write path untested — add limit-0 runs. R1-4 (minor) case 3 must pin names+bytes, serialized size 4097–16384, setup before fork. R1-5 (minor) add llmevaluator.cpp:132 to backlog; correct hex-export reason. R1-6 (minor) saveRecovery is a live writer with no reader. R1-7 (minor) restoreOriginal comment becomes false; decide on size check. R1-8 (minor) add T1.4 5f3a2ee to fixes_artifact.

spec_verdict: docs/superpowers/specs/2026-11-28-atomic-short-write.md — Umar round 1, verdict: revise

Orchestrator: all eight applied; R1-1 option (a) chosen; R1-7 size check kept. Index in spec §10.

## Round 2 — Umar — scoped re-check — verdict: approve

- spec_sha256 (round 2 input): `3ea31e9589a1d22010323eb088706a9d78b8a8bd3770c04e4a4ccec923f4cca5`
- Tested the design as written with probe `scratchpad/umar-p112/r2.cpp` (Qt 6.2.4): EIO shim → failure, original kept, "Input/output error"; 1000-byte write cap → success (no false failures); RLIMIT 4096 sizes 4097–65536 → failure, original kept, "File too large" (errno survives Qt's short write); limit 0 → same. `qt_error_string` is public (qglobal.h:991).
- R1-1 closed · R1-2 closed (round 2). R1-3..R1-8 author notes.
- Notes: N1 commit or paste the shim source (scratchpad not durable); N2 clear errno before write as a safeguard.

spec_verdict: docs/superpowers/specs/2026-11-28-atomic-short-write.md — Umar round 2, verdict: approve

Orchestrator: N1, N2 applied after approval.

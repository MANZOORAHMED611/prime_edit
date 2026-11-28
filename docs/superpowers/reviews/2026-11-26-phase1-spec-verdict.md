# Spec verdict — Phase 1 data-loss fixes

spec: docs/superpowers/specs/2026-11-26-phase1-data-loss-fixes.md
tier: T3
reviewer: Umar

## Round 1 — 2026-11-27T04:09Z — verdict: revise
Blockers:
- R1-1: the shared-QTextDocument Clone is unsound. The clone's own document and its highlighter are destroyed, and closing the original causes a use-after-free (reproduced under ASan).
- R1-2: the workstreams were not file-disjoint, and the breakdown lacked agents, dependencies and commands.
- R1-3: restoring into the wrong editor and dropping corrupt entries were not in scope.

Material: R1-4 to R1-13 covered:
- the sync-flag target per site, and the format-dirty flag;
- QSaveFile edge cases;
- IPC, the recovery table and the socket name;
- toPlainText dropping NBSP and U+2028, which became new defect D11;
- the definition of lossless;
- the test gaps;
- read-only being advisory, which became new defect D12;
- the tier header;
- the live-instance install hazard;
- the fold-state contract.

Minor: R1-14 to R1-17.

## Round 2 (scoped re-check of fixes) — 2026-11-27T04:58Z — verdict: approve
spec_sha256 at review: fcba8a3bb2d3c638ada57bebaaebe1b8a696fa12a02cc3ee77f2b6d473a86116
- R1-1 to R1-13 are closed in round 2.
- Four minor notes, all applied before dispatch:
  - R2-1: placeholder bodies for contracts that are declared but not yet defined;
  - R2-2: the scope check runs over non-merge commits;
  - R2-3: hard-link and foreign-owner saves write directly with QFile;
  - R2-4: corrected the wording of index row R1-13.
- Probes run in scratch, Qt 6.2.4:
  - QLockFile is not taken from a live holder;
  - toRawText() with U+2029 mapped to LF round-trips U+00A0 and U+2028;
  - the rollback .deb sha256 was verified.

Brief header: `spec_verdict: docs/superpowers/specs/2026-11-26-phase1-data-loss-fixes.md — Umar round 2, verdict: approve`

# Output gate: P1.12-A (atomic saves detect a failed write or sync) — Umar — validated, 10/10

- Spec: `docs/superpowers/specs/2026-11-28-atomic-short-write.md`
- Branch `fix/p112-short-write`: `a99b68d` (failing tests), `0c94d52` (fix), `947af33` (backlog), `2dffaba` (EIO shim + driver for case 5). Base `800d39e`.
- Independent builds from git-archive exports. Fail-first at a99b68d: 3 pass / 5 fail exactly as receipted (sizes 4097–16384 corrupt to 4096 bytes reported Ok; limit-0 leaves 0 bytes; 16385+ "Unknown error"; draft cache case fails). Case 5 fail-first added by the gate: driver against the old library under the EIO shim changes the file and reports success.
- After the fix: Release 23/23; ASan/UBSan 23/23, 0 reports (re-run verbose); function length OK; strace shows one payload write, the helper's fdatasync, Qt's fdatasync, linkat, rename; EIO shim → IoError "Input/output error" / writeFileAtomic false, file sha1 unchanged, no temp file left.
- Stale-errno analysis: no path; lastError equals strerror(EFBIG) at all sizes and strerror(EIO) under the shim.
- Backlog citations verified at HEAD.
- Notes: unused `armed()` in the shim (copied verbatim from the gate's harness); untracked build dirs and a hook-written SESSION.md in the builder worktree.

output_gate: P1.12-A — Umar — verdict: validated, score 10/10

# Output gate: P1.11-A (direct write + chunked decode verify) — Umar — validated, 9.3/10

- Spec: `docs/superpowers/specs/2026-11-27-pr5-review-data-safety.md` (§3.1, §3.5, §5 cases 1–4b, 10, memory and strace receipts, §8 task A)
- Branch `fix/p111-direct-write`: `a3d16b3` (failing tests), `2192b2b` (direct write), `f92e902` (chunked decode verify), `eb35f72` (guide). Base `ee09368`.
- Independent builds: Release 21/21; ASan/UBSan 21/21, 0 reports (also re-run verbose for the touched suites: 0); fail-first at a3d16b3: cases 1, 2, 3, 4, 4b fail on real truncation; case 10 passes before the fix by design (verdict-preservation guard; F5's fail-first evidence is the memory receipt); function length OK on ee09368 and fdd5867.
- Memory receipt reproduced: UTF-8 404→305, windows-1252 407→307, UTF-8-BOM 504→305 MiB, all lossless=1 (target ≤ 320 MiB).
- strace receipt reproduced with an independent driver: injected EOPNOTSUPP → IoError/Restored/identical; uninjected → fallocate EFBIG, IoError/Unchanged/identical; no NUL pwrite.
- Mutations: removing the recovery-copy size check fails case 4b (confirms the P1.12 short-write bug and that A guards it); four case-10 decoder mutants all caught. Differential fuzz old vs new decode: 104,000 cases, 26 encodings, identical at the real slice size.
- Amendments accepted: case 10 windows-1252 altered/trailing expect lossless (ICU cp1252 maps all 256 bytes), lossy single-byte legs on windows-1253; ISO-2022-JP `ESC ( J` variant.
- Findings (minor): (1) guide paths `~/.local/share/PrimeEdit/sessions/...` omit the doubled org/app directory — real AppDataLocation is `~/.local/share/PrimeEdit/PrimeEdit` (inherited from existing rows; fixed guide-wide by the orchestrator after the gate); (2) SCSU > 1 Mi code units can flip to lossy under chunking — unreachable encoding, read-only only. Cosmetic: empty `unsaved.recovered/` left when a short copy is dropped.

output_gate: P1.11-A — Umar — verdict: validated, score 9.3/10

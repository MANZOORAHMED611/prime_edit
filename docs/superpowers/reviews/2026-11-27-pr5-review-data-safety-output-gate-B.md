# Output gate: P1.11-B (drafts and quit) — Umar — validated, 10.0/10

- Spec: `docs/superpowers/specs/2026-11-27-pr5-review-data-safety.md` (§3.2–3.4, §5 cases 5–9, §8 task B)
- Branch `fix/p111-drafts`, commits `ec97a83` (failing tests), `3589c44` (fix), base `ee09368`
- Independent builds (git archive exports): Release 21/21; ASan/UBSan 21/21, 0 reports (instrumentation confirmed); fail-first at ec97a83 8/10 failing as receipted; mutation run (remove `|| entries.isEmpty()` and the restore guard) → cases 6b and 7 fail, so both rules are discriminated; 4 related suites green over 5 repeats; function length OK on ee09368 and fdd5867; pinned UI strings match exactly; no debug output.
- DoD: all items evidenced. Task-A reserved areas untouched; guide edits inside #recovery only.
- Deviation accepted: case 9 fixture (spec fixture never opens read-only via openFile; builder's UTF-16LE lone-surrogate file does). Spec §5 case 9 amended accordingly.
- Observations (not corrections): §3.4 isPartialView branch untested (not required); cosmetic garbled comment at test_t1_11b_drafts.cpp:74; a close during a restore modal quits without prompt, as §3.3.2 specifies (on-disk set intact).

output_gate: P1.11-B — Umar — verdict: validated, score 10/10

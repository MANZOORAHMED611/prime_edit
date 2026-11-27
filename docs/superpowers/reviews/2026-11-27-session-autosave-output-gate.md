# Output gate — P1.10-A session autosave

gate_agent: umar · gate_result: **validated** · corrections: none
builder: uthman · score: 10.0 (accuracy 10, completeness 10, constraints 10, consistency 10, autonomy 10)
branch: fix/session-autosave (83923d4 placeholders + failing suite, 6ce3a11 impl, df94875 docs), base 13c7afb

Evidence (re-run by the gate, not the builder):
- Release ctest: 100% passed, 0 failed out of 20 (test_t1_10_session_autosave 14.66 s)
- ASan+UBSan: T1.4 6/6, T1.10 10/10, 0 sanitizer reports
- Independent fail-first at 83923d4: cases 1-5 FAIL, 6-8 PASS (matches builder log)
- `tools/check_function_length.py --base 64552c5`: OK
- DoD: all six items evidenced; three declared builder decisions accepted

Follow-up applied by the orchestrator: guide.html:699 and :784 now use the real
preference label "Restore previous session on startup" (observation, not scored).

# rewrite.professional with qwen3.5-0.8b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 00:04.

| group | n | ttft_s_median | total_s_median | gen_tps_median | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.263 | 1.962 | 34.9 | 93.8 | 77.6 | 0.0 | 1.23 | rw-en-001, rw-en-002, rw-en-003, rw-en-004, rw-en-006, rw-en-011, rw-en-012, rw-en-013, rw-en-020, rw-en-022, rw-en-023, rw-en-025, rw-en-026, rw-en-028, rw-en-030, rw-en-031, rw-en-032, rw-en-034, rw-en-037, rw-en-039, rw-en-041, rw-en-044, rw-en-048, rw-hi-009, rw-hi-010, rw-hi-011, rw-hi-012, rw-hi-013, rw-hi-014, rw-hi-015 |
| lang=en | 50 | 0.287 | 2.08 | 33.6 | 94.0 | 81.9 | 0.0 | 1.27 | rw-en-001, rw-en-002, rw-en-003, rw-en-004, rw-en-006, rw-en-011, rw-en-012, rw-en-013, rw-en-020, rw-en-022, rw-en-023, rw-en-025, rw-en-026, rw-en-028, rw-en-030, rw-en-031, rw-en-032, rw-en-034, rw-en-037, rw-en-039, rw-en-041, rw-en-044, rw-en-048 |
| lang=hi | 8 | 0.259 | 1.475 | 36.2 | 87.5 | 47.1 | 0.0 | 1.08 | none |
| lang=hinglish | 7 | 0.203 | 1.573 | 37.3 | 100.0 | 60.0 | 0.0 | 1.31 | rw-hi-009, rw-hi-010, rw-hi-011, rw-hi-012, rw-hi-013, rw-hi-014, rw-hi-015 |
| set=generated | 65 | 0.263 | 1.962 | 34.9 | 93.8 | 77.6 | 0.0 | 1.23 | rw-en-001, rw-en-002, rw-en-003, rw-en-004, rw-en-006, rw-en-011, rw-en-012, rw-en-013, rw-en-020, rw-en-022, rw-en-023, rw-en-025, rw-en-026, rw-en-028, rw-en-030, rw-en-031, rw-en-032, rw-en-034, rw-en-037, rw-en-039, rw-en-041, rw-en-044, rw-en-048, rw-hi-009, rw-hi-010, rw-hi-011, rw-hi-012, rw-hi-013, rw-hi-014, rw-hi-015 |

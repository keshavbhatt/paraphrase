# rewrite.professional with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 1, run 2026-10-22 03:38.

| group | n | ttft_s_median | total_s_median | gen_tps_median | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.785 | 2.192 | 21.2 | 98.5 | 86.9 | 0.0 | 1.18 | rw-hi-002, rw-hi-009, rw-hi-010, rw-hi-013, rw-hi-014, rw-hi-015 |
| lang=en | 50 | 0.714 | 2.284 | 21.1 | 98.0 | 87.9 | 0.0 | 1.21 | none |
| lang=hi | 8 | 0.814 | 2.582 | 21.1 | 100.0 | 70.6 | 0.0 | 1.16 | rw-hi-002 |
| lang=hinglish | 7 | 0.874 | 1.932 | 21.4 | 100.0 | 93.3 | 0.0 | 1.05 | rw-hi-009, rw-hi-010, rw-hi-013, rw-hi-014, rw-hi-015 |
| set=generated | 65 | 0.785 | 2.192 | 21.2 | 98.5 | 86.9 | 0.0 | 1.18 | rw-hi-002, rw-hi-009, rw-hi-010, rw-hi-013, rw-hi-014, rw-hi-015 |

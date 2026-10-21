# rewrite.professional with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 1, run 2026-10-22 02:18.

| group | n | ttft_s_median | total_s_median | gen_tps_median | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 1.368 | 4.191 | 11.9 | 100.0 | 90.7 | 0.0 | 1.25 | rw-hi-005, rw-hi-009, rw-hi-010, rw-hi-013 |
| lang=en | 50 | 1.294 | 4.192 | 12.0 | 100.0 | 89.6 | 0.0 | 1.25 | none |
| lang=hi | 8 | 1.374 | 4.103 | 11.8 | 100.0 | 94.1 | 0.0 | 1.24 | rw-hi-005 |
| lang=hinglish | 7 | 1.59 | 3.566 | 11.8 | 100.0 | 100.0 | 0.0 | 1.15 | rw-hi-009, rw-hi-010, rw-hi-013 |
| set=generated | 65 | 1.368 | 4.191 | 11.9 | 100.0 | 90.7 | 0.0 | 1.25 | rw-hi-005, rw-hi-009, rw-hi-010, rw-hi-013 |

# rewrite.professional with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 1, run 2026-10-22 01:37.

| group | n | ttft_s_median | total_s_median | gen_tps_median | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.702 | 2.248 | 21.3 | 98.5 | 88.8 | 0.0 | 1.23 | rw-hi-009, rw-hi-010, rw-hi-013, rw-hi-014, rw-hi-015 |
| lang=en | 50 | 0.685 | 2.426 | 21.1 | 98.0 | 90.1 | 0.0 | 1.25 | none |
| lang=hi | 8 | 0.777 | 2.092 | 21.6 | 100.0 | 70.6 | 0.0 | 1.15 | none |
| lang=hinglish | 7 | 0.751 | 1.695 | 21.8 | 100.0 | 93.3 | 0.0 | 1.05 | rw-hi-009, rw-hi-010, rw-hi-013, rw-hi-014, rw-hi-015 |
| set=generated | 65 | 0.702 | 2.248 | 21.3 | 98.5 | 88.8 | 0.0 | 1.23 | rw-hi-009, rw-hi-010, rw-hi-013, rw-hi-014, rw-hi-015 |

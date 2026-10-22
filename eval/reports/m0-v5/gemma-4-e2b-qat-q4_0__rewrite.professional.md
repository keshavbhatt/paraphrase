# rewrite.professional with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 13:15.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 81 | 0.701 | 2.327 | 21.1 | 8.6 | 100.0 | 88.3 | 0.0 | 1.25 | rw-hi-002, rw-hi-009, rw-hi-010, rw-hi-013, rw-hi-014, rw-hi-015 |
| lang=bn | 2 | 0.572 | 1.493 | 21.7 | 0.0 | 100.0 | 0.0 | 0.0 | 1.57 | none |
| lang=de | 2 | 0.519 | 1.423 | 22.1 | 0.0 | 100.0 | 0.0 | 0.0 | 1.12 | none |
| lang=en | 50 | 0.728 | 2.459 | 20.9 | 0.0 | 100.0 | 89.6 | 0.0 | 1.28 | none |
| lang=es | 2 | 0.464 | 1.483 | 22.1 | 0.0 | 100.0 | 0.0 | 0.0 | 1.28 | none |
| lang=fr | 2 | 0.493 | 1.51 | 22.2 | 0.0 | 100.0 | 0.0 | 0.0 | 1.37 | none |
| lang=hi | 8 | 0.748 | 2.442 | 21.7 | 12.5 | 100.0 | 70.6 | 0.0 | 1.2 | rw-hi-002 |
| lang=hinglish | 7 | 0.867 | 3.145 | 21.7 | 71.4 | 100.0 | 93.3 | 0.0 | 1.05 | rw-hi-009, rw-hi-010, rw-hi-013, rw-hi-014, rw-hi-015 |
| lang=mr | 2 | 1.452 | 2.314 | 20.1 | 0.0 | 100.0 | 0.0 | 0.0 | 1.18 | none |
| lang=pt | 2 | 0.77 | 3.33 | 12.6 | 0.0 | 100.0 | 0.0 | 0.0 | 1.32 | none |
| lang=ta | 2 | 0.506 | 2.244 | 21.3 | 50.0 | 100.0 | 0.0 | 0.0 | 1.24 | none |
| lang=zh | 2 | 0.458 | 1.121 | 22.6 | 0.0 | 100.0 | 0.0 | 0.0 | 1.0 | none |
| set=generated | 81 | 0.701 | 2.327 | 21.1 | 8.6 | 100.0 | 88.3 | 0.0 | 1.25 | rw-hi-002, rw-hi-009, rw-hi-010, rw-hi-013, rw-hi-014, rw-hi-015 |

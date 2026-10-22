# rewrite.professional with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 13:15.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 81 | 1.366 | 4.226 | 10.8 | 2.5 | 100.0 | 90.7 | 0.0 | 1.23 | rw-hi-005, rw-hi-010 |
| lang=bn | 2 | 1.004 | 3.074 | 10.6 | 0.0 | 100.0 | 0.0 | 0.0 | 1.82 | none |
| lang=de | 2 | 1.143 | 2.889 | 10.6 | 0.0 | 100.0 | 0.0 | 0.0 | 1.16 | none |
| lang=en | 50 | 1.345 | 4.56 | 10.9 | 0.0 | 100.0 | 89.6 | 0.0 | 1.24 | none |
| lang=es | 2 | 1.019 | 3.171 | 10.7 | 0.0 | 100.0 | 0.0 | 0.0 | 1.32 | none |
| lang=fr | 2 | 1.084 | 3.317 | 10.6 | 0.0 | 100.0 | 0.0 | 0.0 | 1.45 | none |
| lang=hi | 8 | 1.576 | 4.197 | 10.9 | 12.5 | 100.0 | 100.0 | 0.0 | 1.22 | rw-hi-005 |
| lang=hinglish | 7 | 2.415 | 5.804 | 10.6 | 14.3 | 100.0 | 93.3 | 0.0 | 1.15 | rw-hi-010 |
| lang=mr | 2 | 1.073 | 3.042 | 11.0 | 0.0 | 100.0 | 0.0 | 0.0 | 1.34 | none |
| lang=pt | 2 | 2.077 | 6.315 | 8.2 | 0.0 | 100.0 | 0.0 | 0.0 | 1.59 | none |
| lang=ta | 2 | 1.543 | 5.923 | 7.3 | 0.0 | 100.0 | 0.0 | 0.0 | 1.33 | none |
| lang=zh | 2 | 1.418 | 2.807 | 11.6 | 0.0 | 100.0 | 0.0 | 0.0 | 1.0 | none |
| set=generated | 81 | 1.366 | 4.226 | 10.8 | 2.5 | 100.0 | 90.7 | 0.0 | 1.23 | rw-hi-005, rw-hi-010 |

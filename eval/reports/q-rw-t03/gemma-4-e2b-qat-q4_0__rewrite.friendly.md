# rewrite.friendly with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 21:29.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.092 | 1.951 | 35.0 | 0.0 | 87.5 | 100.0 | 0.0 | 1.21 | none |
| lang=en | 18 | 1.042 | 2.056 | 33.8 | 0.0 | 88.9 | 100.0 | 0.0 | 1.31 | none |
| lang=hi | 3 | 1.358 | 2.102 | 32.3 | 0.0 | 66.7 | 100.0 | 0.0 | 1.08 | none |
| lang=hinglish | 3 | 1.171 | 1.722 | 43.9 | 0.0 | 100.0 | 100.0 | 0.0 | 1.07 | none |
| set=hard | 24 | 1.092 | 1.951 | 35.0 | 0.0 | 87.5 | 100.0 | 0.0 | 1.21 | none |

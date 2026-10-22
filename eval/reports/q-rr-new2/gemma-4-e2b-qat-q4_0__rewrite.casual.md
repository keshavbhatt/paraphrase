# rewrite.casual with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:31.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.131 | 1.851 | 32.6 | 0.0 | 91.7 | 98.7 | 0.0 | 1.06 | none |
| lang=en | 18 | 1.057 | 1.829 | 32.2 | 0.0 | 94.4 | 100.0 | 0.0 | 1.07 | none |
| lang=hi | 3 | 1.215 | 1.895 | 31.7 | 0.0 | 100.0 | 92.9 | 0.0 | 1.0 | none |
| lang=hinglish | 3 | 1.237 | 1.854 | 37.6 | 0.0 | 66.7 | 100.0 | 0.0 | 1.0 | none |
| set=hard | 24 | 1.131 | 1.851 | 32.6 | 0.0 | 91.7 | 98.7 | 0.0 | 1.06 | none |

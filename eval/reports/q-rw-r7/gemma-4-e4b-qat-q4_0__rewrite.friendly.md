# rewrite.friendly with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 20:42.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.858 | 3.54 | 19.0 | 0.0 | 100.0 | 92.2 | 0.0 | 1.28 | none |
| lang=en | 18 | 1.799 | 3.31 | 19.1 | 0.0 | 100.0 | 100.0 | 0.0 | 1.35 | none |
| lang=hi | 3 | 2.007 | 3.36 | 17.9 | 0.0 | 100.0 | 57.1 | 0.0 | 1.17 | none |
| lang=hinglish | 3 | 2.295 | 3.811 | 19.0 | 0.0 | 100.0 | 100.0 | 0.0 | 1.14 | none |
| set=hard | 24 | 1.858 | 3.54 | 19.0 | 0.0 | 100.0 | 92.2 | 0.0 | 1.28 | none |

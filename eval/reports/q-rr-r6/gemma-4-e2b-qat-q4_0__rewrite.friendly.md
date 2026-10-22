# rewrite.friendly with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 20:15.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.191 | 1.923 | 38.4 | 0.0 | 70.8 | 98.7 | 8.3 | 1.05 | none |
| lang=en | 18 | 1.132 | 1.847 | 37.7 | 0.0 | 83.3 | 100.0 | 11.1 | 1.09 | none |
| lang=hi | 3 | 1.224 | 1.664 | 47.8 | 0.0 | 33.3 | 92.9 | 0.0 | 1.0 | none |
| lang=hinglish | 3 | 1.749 | 3.083 | 33.9 | 0.0 | 33.3 | 100.0 | 0.0 | 1.0 | none |
| set=hard | 24 | 1.191 | 1.923 | 38.4 | 0.0 | 70.8 | 98.7 | 8.3 | 1.05 | none |

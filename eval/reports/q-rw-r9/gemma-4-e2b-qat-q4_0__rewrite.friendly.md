# rewrite.friendly with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 21:46.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.626 | 3.276 | 13.4 | 8.3 | 83.3 | 100.0 | 0.0 | 1.17 | none |
| lang=en | 18 | 1.536 | 3.492 | 10.5 | 0.0 | 88.9 | 100.0 | 0.0 | 1.24 | none |
| lang=hi | 3 | 1.894 | 5.083 | 20.1 | 66.7 | 66.7 | 100.0 | 0.0 | 1.19 | none |
| lang=hinglish | 3 | 1.633 | 2.563 | 25.8 | 0.0 | 66.7 | 100.0 | 0.0 | 1.07 | none |
| set=hard | 24 | 1.626 | 3.276 | 13.4 | 8.3 | 83.3 | 100.0 | 0.0 | 1.17 | none |

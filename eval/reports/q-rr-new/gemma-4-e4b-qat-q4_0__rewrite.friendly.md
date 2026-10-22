# rewrite.friendly with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:19.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.856 | 3.261 | 20.0 | 4.2 | 100.0 | 100.0 | 0.0 | 1.23 | none |
| lang=en | 18 | 1.73 | 3.169 | 18.2 | 0.0 | 100.0 | 100.0 | 0.0 | 1.3 | none |
| lang=hi | 3 | 2.186 | 3.334 | 21.4 | 33.3 | 100.0 | 100.0 | 0.0 | 1.0 | none |
| lang=hinglish | 3 | 2.165 | 3.346 | 20.8 | 0.0 | 100.0 | 100.0 | 0.0 | 1.14 | none |
| set=hard | 24 | 1.856 | 3.261 | 20.0 | 4.2 | 100.0 | 100.0 | 0.0 | 1.23 | none |

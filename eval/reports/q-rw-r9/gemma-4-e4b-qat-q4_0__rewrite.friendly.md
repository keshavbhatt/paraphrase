# rewrite.friendly with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 21:41.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 3.908 | 9.617 | 5.3 | 0.0 | 100.0 | 93.5 | 0.0 | 1.19 | none |
| lang=en | 18 | 3.467 | 11.984 | 5.3 | 0.0 | 100.0 | 100.0 | 0.0 | 1.29 | none |
| lang=hi | 3 | 3.947 | 8.93 | 6.8 | 0.0 | 100.0 | 64.3 | 0.0 | 1.17 | none |
| lang=hinglish | 3 | 4.103 | 9.095 | 5.1 | 0.0 | 100.0 | 100.0 | 0.0 | 1.07 | none |
| set=hard | 24 | 3.908 | 9.617 | 5.3 | 0.0 | 100.0 | 93.5 | 0.0 | 1.19 | none |

# rewrite.friendly with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:34.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.055 | 1.742 | 36.6 | 0.0 | 87.5 | 94.8 | 4.2 | 1.18 | none |
| lang=en | 18 | 0.967 | 1.696 | 35.6 | 0.0 | 88.9 | 100.0 | 5.6 | 1.27 | none |
| lang=hi | 3 | 1.072 | 1.742 | 36.7 | 0.0 | 66.7 | 71.4 | 0.0 | 1.0 | none |
| lang=hinglish | 3 | 1.225 | 1.822 | 41.8 | 0.0 | 100.0 | 100.0 | 0.0 | 1.07 | none |
| set=hard | 24 | 1.055 | 1.742 | 36.6 | 0.0 | 87.5 | 94.8 | 4.2 | 1.18 | none |

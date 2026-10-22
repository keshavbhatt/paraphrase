# rewrite.friendly with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 18:49.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.095 | 3.338 | 15.7 | 8.3 | 100.0 | 83.1 | 0.0 | 1.4 | none |
| lang=en | 18 | 1.005 | 2.836 | 15.7 | 0.0 | 100.0 | 88.1 | 0.0 | 1.45 | none |
| lang=hi | 3 | 1.513 | 3.555 | 18.1 | 33.3 | 100.0 | 57.1 | 0.0 | 1.08 | none |
| lang=hinglish | 3 | 1.899 | 3.602 | 14.7 | 33.3 | 100.0 | 100.0 | 0.0 | 1.21 | none |
| set=hard | 24 | 1.095 | 3.338 | 15.7 | 8.3 | 100.0 | 83.1 | 0.0 | 1.4 | none |

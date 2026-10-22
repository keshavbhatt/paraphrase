# rewrite.casual with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:51.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 0.987 | 1.515 | 41.5 | 10.0 | 90.0 | 87.5 | 10.0 | 1.04 | none |
| lang=en | 7 | 0.928 | 1.442 | 43.5 | 0.0 | 100.0 | 100.0 | 14.3 | 1.0 | none |
| lang=hi | 2 | 1.037 | 2.17 | 36.5 | 50.0 | 50.0 | 60.0 | 0.0 | 1.04 | none |
| lang=hinglish | 1 | 1.217 | 1.983 | 28.7 | 0.0 | 100.0 | 0.0 | 0.0 | 1.17 | none |
| set=holdout | 10 | 0.987 | 1.515 | 41.5 | 10.0 | 90.0 | 87.5 | 10.0 | 1.04 | none |

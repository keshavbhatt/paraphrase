# rewrite.casual with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:57.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 0.511 | 1.32 | 38.1 | 20.0 | 100.0 | 81.2 | 10.0 | 1.19 | none |
| lang=en | 7 | 0.478 | 1.046 | 38.5 | 0.0 | 100.0 | 90.9 | 14.3 | 1.22 | none |
| lang=hi | 2 | 0.819 | 2.9 | 32.8 | 100.0 | 100.0 | 60.0 | 0.0 | 1.08 | none |
| lang=hinglish | 1 | 1.276 | 2.689 | 15.6 | 0.0 | 100.0 | 0.0 | 0.0 | 1.17 | none |
| set=holdout | 10 | 0.511 | 1.32 | 38.1 | 20.0 | 100.0 | 81.2 | 10.0 | 1.19 | none |

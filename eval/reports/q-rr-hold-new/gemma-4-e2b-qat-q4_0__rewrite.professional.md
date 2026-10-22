# rewrite.professional with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:51.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 1.005 | 1.645 | 38.0 | 10.0 | 90.0 | 87.5 | 0.0 | 1.07 | none |
| lang=en | 7 | 0.959 | 1.57 | 37.8 | 0.0 | 100.0 | 100.0 | 0.0 | 1.06 | none |
| lang=hi | 2 | 1.05 | 2.07 | 39.5 | 50.0 | 100.0 | 60.0 | 0.0 | 1.19 | none |
| lang=hinglish | 1 | 1.254 | 1.691 | 43.4 | 0.0 | 0.0 | 0.0 | 0.0 | 1.0 | none |
| set=holdout | 10 | 1.005 | 1.645 | 38.0 | 10.0 | 90.0 | 87.5 | 0.0 | 1.07 | none |

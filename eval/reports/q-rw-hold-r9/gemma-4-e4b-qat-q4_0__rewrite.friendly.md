# rewrite.friendly with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 21:49.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 1.854 | 3.297 | 19.0 | 20.0 | 100.0 | 96.9 | 10.0 | 1.29 | none |
| lang=en | 7 | 1.798 | 3.191 | 17.2 | 0.0 | 100.0 | 100.0 | 14.3 | 1.47 | none |
| lang=hi | 2 | 2.016 | 3.886 | 21.3 | 50.0 | 100.0 | 90.0 | 0.0 | 1.04 | none |
| lang=hinglish | 1 | 2.355 | 5.291 | 20.9 | 100.0 | 100.0 | 0.0 | 0.0 | 1.25 | none |
| set=holdout | 10 | 1.854 | 3.297 | 19.0 | 20.0 | 100.0 | 96.9 | 10.0 | 1.29 | none |

# rewrite.casual with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:58.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 1.017 | 2.632 | 19.0 | 20.0 | 100.0 | 96.9 | 10.0 | 1.19 | rw-hi-203 |
| lang=en | 7 | 0.923 | 2.05 | 18.6 | 0.0 | 100.0 | 100.0 | 14.3 | 1.22 | none |
| lang=hi | 2 | 1.477 | 5.359 | 16.8 | 100.0 | 100.0 | 90.0 | 0.0 | 1.08 | rw-hi-203 |
| lang=hinglish | 1 | 1.62 | 2.619 | 23.0 | 0.0 | 100.0 | 0.0 | 0.0 | 1.25 | none |
| set=holdout | 10 | 1.017 | 2.632 | 19.0 | 20.0 | 100.0 | 96.9 | 10.0 | 1.19 | rw-hi-203 |

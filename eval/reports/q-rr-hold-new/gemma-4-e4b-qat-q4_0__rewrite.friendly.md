# rewrite.friendly with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:55.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 1.78 | 3.104 | 18.7 | 0.0 | 100.0 | 87.5 | 10.0 | 1.33 | none |
| lang=en | 7 | 1.727 | 3.108 | 19.7 | 0.0 | 100.0 | 100.0 | 14.3 | 1.44 | none |
| lang=hi | 2 | 1.78 | 2.79 | 20.8 | 0.0 | 100.0 | 60.0 | 0.0 | 1.08 | none |
| lang=hinglish | 1 | 2.121 | 3.444 | 17.4 | 0.0 | 100.0 | 0.0 | 0.0 | 1.33 | none |
| set=holdout | 10 | 1.78 | 3.104 | 18.7 | 0.0 | 100.0 | 87.5 | 10.0 | 1.33 | none |

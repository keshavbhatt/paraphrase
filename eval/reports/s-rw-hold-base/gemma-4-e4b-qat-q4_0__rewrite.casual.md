# rewrite.casual with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 22:36.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 12 | 1.693 | 2.98 | 21.1 | 8.3 | 100.0 | 91.3 | 0.0 | 1.17 | none |
| lang=en | 9 | 1.564 | 2.955 | 20.9 | 0.0 | 100.0 | 100.0 | 0.0 | 1.22 | none |
| lang=hi | 2 | 1.693 | 3.395 | 23.2 | 50.0 | 100.0 | 60.0 | 0.0 | 1.0 | none |
| lang=hinglish | 1 | 2.026 | 3.005 | 21.4 | 0.0 | 100.0 | 0.0 | 0.0 | 1.17 | none |
| set=holdout | 12 | 1.693 | 2.98 | 21.1 | 8.3 | 100.0 | 91.3 | 0.0 | 1.17 | none |

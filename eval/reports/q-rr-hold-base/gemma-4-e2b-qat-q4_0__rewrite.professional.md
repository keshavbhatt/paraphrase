# rewrite.professional with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:56.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 0.511 | 1.266 | 37.2 | 0.0 | 100.0 | 78.1 | 0.0 | 1.32 | none |
| lang=en | 7 | 0.46 | 1.307 | 37.4 | 0.0 | 100.0 | 81.8 | 0.0 | 1.33 | none |
| lang=hi | 2 | 0.585 | 1.187 | 36.6 | 0.0 | 100.0 | 70.0 | 0.0 | 1.23 | none |
| lang=hinglish | 1 | 0.828 | 1.393 | 37.2 | 0.0 | 100.0 | 0.0 | 0.0 | 1.17 | none |
| set=holdout | 10 | 0.511 | 1.266 | 37.2 | 0.0 | 100.0 | 78.1 | 0.0 | 1.32 | none |

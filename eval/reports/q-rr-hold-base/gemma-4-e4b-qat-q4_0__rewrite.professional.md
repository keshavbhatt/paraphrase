# rewrite.professional with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:58.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 1.162 | 2.408 | 19.1 | 10.0 | 100.0 | 75.0 | 0.0 | 1.35 | none |
| lang=en | 7 | 1.022 | 2.402 | 18.2 | 0.0 | 100.0 | 81.8 | 0.0 | 1.39 | none |
| lang=hi | 2 | 1.212 | 3.007 | 26.0 | 50.0 | 100.0 | 60.0 | 0.0 | 1.23 | none |
| lang=hinglish | 1 | 1.563 | 2.415 | 24.7 | 0.0 | 100.0 | 0.0 | 0.0 | 1.08 | none |
| set=holdout | 10 | 1.162 | 2.408 | 19.1 | 10.0 | 100.0 | 75.0 | 0.0 | 1.35 | none |

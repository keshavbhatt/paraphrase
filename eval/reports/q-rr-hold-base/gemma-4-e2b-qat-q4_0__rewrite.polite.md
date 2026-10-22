# rewrite.polite with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:57.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 0.513 | 1.44 | 36.4 | 10.0 | 100.0 | 84.4 | 0.0 | 1.32 | none |
| lang=en | 7 | 0.464 | 1.483 | 35.1 | 0.0 | 100.0 | 95.5 | 0.0 | 1.67 | none |
| lang=hi | 2 | 0.587 | 1.579 | 41.6 | 50.0 | 100.0 | 60.0 | 0.0 | 1.15 | none |
| lang=hinglish | 1 | 0.823 | 1.396 | 36.6 | 0.0 | 100.0 | 0.0 | 0.0 | 1.17 | none |
| set=holdout | 10 | 0.513 | 1.44 | 36.4 | 10.0 | 100.0 | 84.4 | 0.0 | 1.32 | none |

# rewrite.friendly with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:57.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 0.513 | 1.512 | 33.6 | 10.0 | 100.0 | 81.2 | 10.0 | 1.28 | none |
| lang=en | 7 | 0.46 | 1.41 | 33.5 | 0.0 | 100.0 | 90.9 | 14.3 | 1.5 | none |
| lang=hi | 2 | 0.588 | 1.577 | 38.1 | 50.0 | 100.0 | 60.0 | 0.0 | 1.04 | none |
| lang=hinglish | 1 | 0.817 | 1.613 | 30.1 | 0.0 | 100.0 | 0.0 | 0.0 | 1.33 | none |
| set=holdout | 10 | 0.513 | 1.512 | 33.6 | 10.0 | 100.0 | 81.2 | 10.0 | 1.28 | none |

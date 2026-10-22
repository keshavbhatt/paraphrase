# rewrite.friendly with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:52.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 0.981 | 1.677 | 37.4 | 10.0 | 90.0 | 87.5 | 10.0 | 1.16 | none |
| lang=en | 7 | 0.909 | 1.516 | 37.9 | 0.0 | 100.0 | 100.0 | 14.3 | 1.17 | none |
| lang=hi | 2 | 1.063 | 2.359 | 28.1 | 50.0 | 100.0 | 60.0 | 0.0 | 1.08 | none |
| lang=hinglish | 1 | 1.199 | 1.637 | 43.3 | 0.0 | 0.0 | 0.0 | 0.0 | 1.0 | none |
| set=holdout | 10 | 0.981 | 1.677 | 37.4 | 10.0 | 90.0 | 87.5 | 10.0 | 1.16 | none |

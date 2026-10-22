# rewrite.friendly with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 21:50.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 1.021 | 1.814 | 35.2 | 10.0 | 100.0 | 87.5 | 10.0 | 1.17 | none |
| lang=en | 7 | 0.966 | 1.798 | 36.6 | 0.0 | 100.0 | 100.0 | 14.3 | 1.28 | none |
| lang=hi | 2 | 1.042 | 2.132 | 34.9 | 50.0 | 100.0 | 60.0 | 0.0 | 1.08 | none |
| lang=hinglish | 1 | 1.231 | 1.83 | 35.0 | 0.0 | 100.0 | 0.0 | 0.0 | 1.17 | none |
| set=holdout | 10 | 1.021 | 1.814 | 35.2 | 10.0 | 100.0 | 87.5 | 10.0 | 1.17 | none |

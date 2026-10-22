# rewrite.casual with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 22:47.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 29 | 1.913 | 3.467 | 20.0 | 3.4 | 96.6 | 95.1 | 0.0 | 1.13 | none |
| lang=en | 22 | 1.791 | 3.247 | 20.2 | 0.0 | 100.0 | 100.0 | 0.0 | 1.15 | none |
| lang=hi | 3 | 1.965 | 3.707 | 19.9 | 33.3 | 100.0 | 57.1 | 0.0 | 1.0 | none |
| lang=hinglish | 4 | 2.378 | 3.608 | 22.6 | 0.0 | 75.0 | 100.0 | 0.0 | 1.06 | none |
| set=hard | 29 | 1.913 | 3.467 | 20.0 | 3.4 | 96.6 | 95.1 | 0.0 | 1.13 | none |

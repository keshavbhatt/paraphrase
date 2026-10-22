# rewrite.casual with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:02.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.108 | 1.984 | 34.9 | 12.5 | 91.7 | 98.7 | 0.0 | 1.0 | rw-hi-101, rw-hi-102 |
| lang=en | 18 | 1.064 | 1.75 | 34.9 | 0.0 | 94.4 | 98.3 | 0.0 | 1.02 | none |
| lang=hi | 3 | 1.354 | 3.96 | 20.0 | 100.0 | 100.0 | 100.0 | 0.0 | 1.0 | rw-hi-101, rw-hi-102 |
| lang=hinglish | 3 | 1.232 | 2.055 | 28.1 | 0.0 | 66.7 | 100.0 | 0.0 | 1.0 | none |
| set=hard | 24 | 1.108 | 1.984 | 34.9 | 12.5 | 91.7 | 98.7 | 0.0 | 1.0 | rw-hi-101, rw-hi-102 |

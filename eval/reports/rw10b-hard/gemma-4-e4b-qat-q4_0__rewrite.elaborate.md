# rewrite.elaborate with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-25 21:29.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 33 | 2.13 | 5.327 | 14.6 | 0.0 | 100.0 | 89.5 | 0.0 | 1.8 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |
| lang=en | 25 | 2.089 | 5.538 | 14.6 | 0.0 | 100.0 | 93.5 | 0.0 | 2.0 | rw-en-123, rw-en-124, rw-en-125 |
| lang=hi | 4 | 1.97 | 4.157 | 15.2 | 0.0 | 100.0 | 62.5 | 0.0 | 1.45 | rw-hi-108 |
| lang=hinglish | 4 | 2.304 | 7.052 | 10.5 | 0.0 | 100.0 | 90.0 | 0.0 | 1.54 | none |
| set=hard | 33 | 2.13 | 5.327 | 14.6 | 0.0 | 100.0 | 89.5 | 0.0 | 1.8 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |

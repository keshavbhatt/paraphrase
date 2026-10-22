# rewrite.concise with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:03.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.174 | 1.664 | 36.3 | 8.3 | 83.3 | 100.0 | 0.0 | 0.79 | rw-hi-102 |
| lang=en | 18 | 1.037 | 1.483 | 35.7 | 0.0 | 94.4 | 100.0 | 0.0 | 0.71 | none |
| lang=hi | 3 | 1.201 | 3.078 | 40.7 | 66.7 | 66.7 | 100.0 | 0.0 | 0.94 | rw-hi-102 |
| lang=hinglish | 3 | 1.372 | 1.923 | 45.1 | 0.0 | 33.3 | 100.0 | 0.0 | 1.0 | none |
| set=hard | 24 | 1.174 | 1.664 | 36.3 | 8.3 | 83.3 | 100.0 | 0.0 | 0.79 | rw-hi-102 |

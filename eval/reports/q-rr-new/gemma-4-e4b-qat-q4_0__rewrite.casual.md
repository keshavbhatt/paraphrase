# rewrite.casual with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:15.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.793 | 3.269 | 19.0 | 12.5 | 95.8 | 100.0 | 0.0 | 1.09 | rw-hi-101, rw-hi-102, rw-hi-103 |
| lang=en | 18 | 1.666 | 3.036 | 19.7 | 0.0 | 100.0 | 100.0 | 0.0 | 1.1 | none |
| lang=hi | 3 | 2.016 | 6.311 | 12.9 | 100.0 | 100.0 | 100.0 | 0.0 | 1.0 | rw-hi-101, rw-hi-102, rw-hi-103 |
| lang=hinglish | 3 | 2.192 | 3.289 | 21.4 | 0.0 | 66.7 | 100.0 | 0.0 | 1.13 | none |
| set=hard | 24 | 1.793 | 3.269 | 19.0 | 12.5 | 95.8 | 100.0 | 0.0 | 1.09 | rw-hi-101, rw-hi-102, rw-hi-103 |

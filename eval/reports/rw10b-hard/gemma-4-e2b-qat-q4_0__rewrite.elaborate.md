# rewrite.elaborate with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-25 21:36.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 33 | 1.124 | 2.427 | 28.9 | 3.0 | 100.0 | 86.5 | 0.0 | 1.56 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |
| lang=en | 25 | 1.076 | 2.351 | 28.9 | 0.0 | 100.0 | 90.7 | 0.0 | 1.69 | rw-en-123, rw-en-124, rw-en-125 |
| lang=hi | 4 | 1.133 | 3.337 | 28.8 | 25.0 | 100.0 | 62.5 | 0.0 | 1.44 | rw-hi-108 |
| lang=hinglish | 4 | 1.301 | 2.718 | 28.2 | 0.0 | 100.0 | 80.0 | 0.0 | 1.35 | none |
| set=hard | 33 | 1.124 | 2.427 | 28.9 | 3.0 | 100.0 | 86.5 | 0.0 | 1.56 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |

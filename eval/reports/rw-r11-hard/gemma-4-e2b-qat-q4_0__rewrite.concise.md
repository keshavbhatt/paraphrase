# rewrite.concise with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-25 21:50.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 33 | 1.127 | 1.67 | 40.7 | 0.0 | 87.9 | 100.0 | 0.0 | 0.86 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |
| lang=en | 25 | 1.082 | 1.572 | 36.0 | 0.0 | 92.0 | 100.0 | 0.0 | 0.8 | rw-en-123, rw-en-124, rw-en-125 |
| lang=hi | 4 | 1.242 | 1.732 | 43.6 | 0.0 | 75.0 | 100.0 | 0.0 | 0.93 | rw-hi-108 |
| lang=hinglish | 4 | 1.245 | 1.693 | 49.3 | 0.0 | 75.0 | 100.0 | 0.0 | 1.0 | none |
| set=hard | 33 | 1.127 | 1.67 | 40.7 | 0.0 | 87.9 | 100.0 | 0.0 | 0.86 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |

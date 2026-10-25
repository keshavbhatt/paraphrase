# rewrite.elaborate with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-25 21:49.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 33 | 1.249 | 2.847 | 27.3 | 6.1 | 100.0 | 85.7 | 0.0 | 1.55 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |
| lang=en | 25 | 1.166 | 2.616 | 27.2 | 0.0 | 100.0 | 91.6 | 0.0 | 1.75 | rw-en-123, rw-en-124, rw-en-125 |
| lang=hi | 4 | 1.692 | 5.009 | 22.2 | 25.0 | 100.0 | 50.0 | 0.0 | 1.5 | rw-hi-108 |
| lang=hinglish | 4 | 1.255 | 2.683 | 28.2 | 25.0 | 100.0 | 80.0 | 0.0 | 1.31 | none |
| set=hard | 33 | 1.249 | 2.847 | 27.3 | 6.1 | 100.0 | 85.7 | 0.0 | 1.55 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |

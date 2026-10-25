# rewrite.elaborate with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-25 21:42.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 33 | 2.142 | 4.565 | 15.5 | 0.0 | 100.0 | 90.2 | 0.0 | 1.76 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |
| lang=en | 25 | 1.896 | 4.41 | 15.5 | 0.0 | 100.0 | 92.5 | 0.0 | 1.92 | rw-en-123, rw-en-124, rw-en-125 |
| lang=hi | 4 | 2.124 | 4.318 | 16.2 | 0.0 | 100.0 | 68.8 | 0.0 | 1.73 | rw-hi-108 |
| lang=hinglish | 4 | 2.44 | 4.85 | 14.6 | 0.0 | 100.0 | 100.0 | 0.0 | 1.45 | none |
| set=hard | 33 | 2.142 | 4.565 | 15.5 | 0.0 | 100.0 | 90.2 | 0.0 | 1.76 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |

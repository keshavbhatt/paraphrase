# rewrite.concise with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-25 21:31.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 33 | 2.075 | 3.003 | 20.8 | 0.0 | 93.9 | 96.2 | 0.0 | 0.77 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |
| lang=en | 25 | 1.835 | 2.915 | 20.4 | 0.0 | 96.0 | 100.0 | 0.0 | 0.73 | rw-en-123, rw-en-124, rw-en-125 |
| lang=hi | 4 | 2.723 | 6.089 | 16.7 | 0.0 | 100.0 | 68.8 | 0.0 | 0.87 | rw-hi-108 |
| lang=hinglish | 4 | 2.202 | 3.018 | 21.6 | 0.0 | 75.0 | 100.0 | 0.0 | 0.83 | none |
| set=hard | 33 | 2.075 | 3.003 | 20.8 | 0.0 | 93.9 | 96.2 | 0.0 | 0.77 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |

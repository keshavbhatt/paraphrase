# rewrite.concise with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-25 21:37.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 33 | 1.16 | 1.81 | 34.0 | 0.0 | 84.8 | 99.2 | 0.0 | 0.83 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |
| lang=en | 25 | 1.109 | 1.772 | 33.5 | 0.0 | 92.0 | 99.1 | 0.0 | 0.79 | rw-en-123, rw-en-124, rw-en-125 |
| lang=hi | 4 | 1.217 | 1.714 | 35.6 | 0.0 | 75.0 | 100.0 | 0.0 | 0.94 | rw-hi-108 |
| lang=hinglish | 4 | 1.46 | 2.087 | 36.8 | 0.0 | 50.0 | 100.0 | 0.0 | 0.97 | none |
| set=hard | 33 | 1.16 | 1.81 | 34.0 | 0.0 | 84.8 | 99.2 | 0.0 | 0.83 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |

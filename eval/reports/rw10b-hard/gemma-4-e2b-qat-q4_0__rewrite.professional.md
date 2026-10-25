# rewrite.professional with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-25 21:38.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 33 | 1.118 | 1.913 | 33.4 | 9.1 | 93.9 | 94.0 | 0.0 | 1.13 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |
| lang=en | 25 | 1.093 | 1.847 | 36.4 | 0.0 | 100.0 | 97.2 | 0.0 | 1.16 | rw-en-123, rw-en-124, rw-en-125 |
| lang=hi | 4 | 1.19 | 2.462 | 29.1 | 25.0 | 100.0 | 68.8 | 0.0 | 1.04 | rw-hi-108 |
| lang=hinglish | 4 | 1.396 | 3.168 | 42.6 | 50.0 | 50.0 | 100.0 | 0.0 | 1.0 | none |
| set=hard | 33 | 1.118 | 1.913 | 33.4 | 9.1 | 93.9 | 94.0 | 0.0 | 1.13 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |

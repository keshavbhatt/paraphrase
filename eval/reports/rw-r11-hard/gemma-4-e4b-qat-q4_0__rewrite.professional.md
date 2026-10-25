# rewrite.professional with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-25 21:47.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 33 | 2.105 | 3.456 | 20.5 | 6.1 | 93.9 | 95.5 | 0.0 | 1.14 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |
| lang=en | 25 | 1.89 | 3.403 | 22.0 | 0.0 | 96.0 | 100.0 | 0.0 | 1.16 | rw-en-123, rw-en-124, rw-en-125 |
| lang=hi | 4 | 3.635 | 6.084 | 15.1 | 0.0 | 100.0 | 62.5 | 0.0 | 1.12 | rw-hi-108 |
| lang=hinglish | 4 | 2.243 | 4.472 | 24.3 | 50.0 | 75.0 | 100.0 | 0.0 | 1.0 | none |
| set=hard | 33 | 2.105 | 3.456 | 20.5 | 6.1 | 93.9 | 95.5 | 0.0 | 1.14 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |

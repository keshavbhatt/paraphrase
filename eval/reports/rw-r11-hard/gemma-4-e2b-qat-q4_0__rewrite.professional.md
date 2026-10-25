# rewrite.professional with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-25 21:51.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 33 | 1.226 | 2.03 | 32.6 | 3.0 | 93.9 | 94.0 | 0.0 | 1.16 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |
| lang=en | 25 | 1.201 | 2.086 | 30.5 | 0.0 | 100.0 | 97.2 | 0.0 | 1.17 | rw-en-123, rw-en-124, rw-en-125 |
| lang=hi | 4 | 1.157 | 1.898 | 35.7 | 0.0 | 100.0 | 68.8 | 0.0 | 1.17 | rw-hi-108 |
| lang=hinglish | 4 | 1.531 | 2.201 | 33.3 | 25.0 | 50.0 | 100.0 | 0.0 | 1.0 | none |
| set=hard | 33 | 1.226 | 2.03 | 32.6 | 3.0 | 93.9 | 94.0 | 0.0 | 1.16 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |

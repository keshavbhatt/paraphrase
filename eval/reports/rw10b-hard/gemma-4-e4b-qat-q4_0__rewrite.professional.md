# rewrite.professional with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-25 21:34.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 33 | 1.882 | 3.483 | 19.9 | 6.1 | 100.0 | 95.5 | 0.0 | 1.12 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |
| lang=en | 25 | 1.827 | 3.388 | 20.9 | 0.0 | 100.0 | 100.0 | 0.0 | 1.16 | rw-en-123, rw-en-124, rw-en-125 |
| lang=hi | 4 | 1.876 | 3.165 | 19.3 | 0.0 | 100.0 | 62.5 | 0.0 | 1.09 | rw-hi-108 |
| lang=hinglish | 4 | 2.57 | 6.009 | 17.7 | 50.0 | 100.0 | 100.0 | 0.0 | 1.03 | none |
| set=hard | 33 | 1.882 | 3.483 | 19.9 | 6.1 | 100.0 | 95.5 | 0.0 | 1.12 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |

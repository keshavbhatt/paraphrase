# rewrite.concise with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-25 21:44.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 33 | 2.083 | 3.106 | 20.3 | 0.0 | 93.9 | 99.2 | 0.0 | 0.76 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |
| lang=en | 25 | 1.919 | 3.008 | 19.7 | 0.0 | 96.0 | 100.0 | 0.0 | 0.68 | rw-en-123, rw-en-124, rw-en-125 |
| lang=hi | 4 | 1.905 | 2.837 | 22.7 | 0.0 | 100.0 | 93.8 | 0.0 | 0.9 | rw-hi-108 |
| lang=hinglish | 4 | 2.768 | 3.675 | 23.5 | 0.0 | 75.0 | 100.0 | 0.0 | 0.84 | none |
| set=hard | 33 | 2.083 | 3.106 | 20.3 | 0.0 | 93.9 | 99.2 | 0.0 | 0.76 | rw-en-123, rw-en-124, rw-en-125, rw-hi-108 |

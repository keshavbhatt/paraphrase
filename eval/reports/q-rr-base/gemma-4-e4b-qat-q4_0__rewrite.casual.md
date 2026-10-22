# rewrite.casual with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 18:45.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.079 | 2.782 | 18.8 | 8.3 | 95.8 | 81.8 | 0.0 | 1.23 | rw-hi-102 |
| lang=en | 18 | 1.008 | 2.763 | 17.8 | 0.0 | 100.0 | 76.3 | 0.0 | 1.26 | none |
| lang=hi | 3 | 1.29 | 4.426 | 18.7 | 66.7 | 100.0 | 100.0 | 0.0 | 1.0 | rw-hi-102 |
| lang=hinglish | 3 | 1.653 | 2.777 | 22.3 | 0.0 | 66.7 | 100.0 | 0.0 | 1.13 | none |
| set=hard | 24 | 1.079 | 2.782 | 18.8 | 8.3 | 95.8 | 81.8 | 0.0 | 1.23 | rw-hi-102 |

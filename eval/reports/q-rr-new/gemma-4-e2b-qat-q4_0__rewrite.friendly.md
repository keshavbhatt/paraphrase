# rewrite.friendly with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:04.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.079 | 1.768 | 33.6 | 8.3 | 91.7 | 100.0 | 0.0 | 1.12 | rw-hi-101, rw-hi-102 |
| lang=en | 18 | 1.052 | 1.826 | 33.2 | 0.0 | 94.4 | 100.0 | 0.0 | 1.19 | none |
| lang=hi | 3 | 1.081 | 2.636 | 32.9 | 66.7 | 66.7 | 100.0 | 0.0 | 1.0 | rw-hi-101, rw-hi-102 |
| lang=hinglish | 3 | 1.236 | 1.747 | 49.0 | 0.0 | 100.0 | 100.0 | 0.0 | 1.07 | none |
| set=hard | 24 | 1.079 | 1.768 | 33.6 | 8.3 | 91.7 | 100.0 | 0.0 | 1.12 | rw-hi-101, rw-hi-102 |

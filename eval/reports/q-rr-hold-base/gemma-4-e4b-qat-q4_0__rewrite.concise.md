# rewrite.concise with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:59.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 1.215 | 1.978 | 16.9 | 10.0 | 100.0 | 81.2 | 0.0 | 0.64 | rw-hi-203 |
| lang=en | 7 | 1.01 | 1.973 | 16.9 | 0.0 | 100.0 | 77.3 | 0.0 | 0.67 | none |
| lang=hi | 2 | 1.224 | 2.87 | 15.0 | 50.0 | 100.0 | 90.0 | 0.0 | 0.62 | rw-hi-203 |
| lang=hinglish | 1 | 1.545 | 1.977 | 30.1 | 0.0 | 100.0 | 0.0 | 0.0 | 0.58 | none |
| set=holdout | 10 | 1.215 | 1.978 | 16.9 | 10.0 | 100.0 | 81.2 | 0.0 | 0.64 | rw-hi-203 |

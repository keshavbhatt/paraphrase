# rewrite.concise with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 21:28.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.133 | 1.675 | 35.1 | 0.0 | 83.3 | 100.0 | 0.0 | 0.79 | none |
| lang=en | 18 | 1.088 | 1.709 | 31.3 | 0.0 | 94.4 | 100.0 | 0.0 | 0.76 | none |
| lang=hi | 3 | 1.153 | 1.737 | 45.2 | 0.0 | 66.7 | 100.0 | 0.0 | 0.88 | none |
| lang=hinglish | 3 | 1.186 | 1.594 | 52.4 | 0.0 | 33.3 | 100.0 | 0.0 | 1.0 | none |
| set=hard | 24 | 1.133 | 1.675 | 35.1 | 0.0 | 83.3 | 100.0 | 0.0 | 0.79 | none |

# rewrite.friendly with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 18:35.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 0.638 | 1.821 | 26.2 | 4.2 | 95.8 | 83.1 | 8.3 | 1.27 | none |
| lang=en | 18 | 0.629 | 1.952 | 25.2 | 0.0 | 100.0 | 86.4 | 11.1 | 1.37 | none |
| lang=hi | 3 | 0.593 | 1.335 | 40.4 | 33.3 | 66.7 | 64.3 | 0.0 | 1.0 | none |
| lang=hinglish | 3 | 0.832 | 1.62 | 29.2 | 0.0 | 100.0 | 100.0 | 0.0 | 1.13 | none |
| set=hard | 24 | 0.638 | 1.821 | 26.2 | 4.2 | 95.8 | 83.1 | 8.3 | 1.27 | none |

# rewrite.friendly with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 20:38.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.041 | 1.725 | 35.0 | 0.0 | 83.3 | 98.7 | 0.0 | 1.22 | none |
| lang=en | 18 | 0.987 | 1.748 | 32.8 | 0.0 | 94.4 | 98.3 | 0.0 | 1.29 | none |
| lang=hi | 3 | 1.13 | 1.779 | 40.3 | 0.0 | 66.7 | 100.0 | 0.0 | 1.08 | none |
| lang=hinglish | 3 | 1.265 | 1.724 | 48.1 | 0.0 | 33.3 | 100.0 | 0.0 | 1.0 | none |
| set=hard | 24 | 1.041 | 1.725 | 35.0 | 0.0 | 83.3 | 98.7 | 0.0 | 1.22 | none |

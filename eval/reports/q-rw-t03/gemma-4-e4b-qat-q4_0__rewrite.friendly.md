# rewrite.friendly with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 21:24.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.801 | 3.382 | 18.1 | 0.0 | 91.7 | 92.2 | 0.0 | 1.22 | none |
| lang=en | 18 | 1.736 | 3.205 | 17.3 | 0.0 | 100.0 | 100.0 | 0.0 | 1.27 | none |
| lang=hi | 3 | 2.182 | 3.22 | 17.5 | 0.0 | 66.7 | 57.1 | 0.0 | 1.0 | none |
| lang=hinglish | 3 | 2.376 | 3.44 | 18.7 | 0.0 | 66.7 | 100.0 | 0.0 | 1.13 | none |
| set=hard | 24 | 1.801 | 3.382 | 18.1 | 0.0 | 91.7 | 92.2 | 0.0 | 1.22 | none |

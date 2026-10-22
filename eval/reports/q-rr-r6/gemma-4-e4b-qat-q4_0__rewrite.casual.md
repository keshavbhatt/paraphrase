# rewrite.casual with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 20:25.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 2.214 | 3.475 | 18.7 | 0.0 | 100.0 | 98.7 | 0.0 | 1.03 | none |
| lang=en | 18 | 2.104 | 3.423 | 18.7 | 0.0 | 100.0 | 98.3 | 0.0 | 1.07 | none |
| lang=hi | 3 | 2.213 | 3.498 | 18.8 | 0.0 | 100.0 | 100.0 | 0.0 | 1.0 | none |
| lang=hinglish | 3 | 2.444 | 3.645 | 19.9 | 0.0 | 100.0 | 100.0 | 0.0 | 1.0 | none |
| set=hard | 24 | 2.214 | 3.475 | 18.7 | 0.0 | 100.0 | 98.7 | 0.0 | 1.03 | none |

# rewrite.casual with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:44.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.834 | 3.173 | 19.6 | 4.2 | 91.7 | 100.0 | 0.0 | 1.14 | none |
| lang=en | 18 | 1.703 | 2.962 | 19.0 | 0.0 | 100.0 | 100.0 | 0.0 | 1.17 | none |
| lang=hi | 3 | 2.065 | 4.838 | 18.8 | 33.3 | 100.0 | 100.0 | 0.0 | 1.0 | none |
| lang=hinglish | 3 | 2.158 | 3.031 | 25.9 | 0.0 | 33.3 | 100.0 | 0.0 | 1.0 | none |
| set=hard | 24 | 1.834 | 3.173 | 19.6 | 4.2 | 91.7 | 100.0 | 0.0 | 1.14 | none |

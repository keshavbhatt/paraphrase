# rewrite.casual with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 21:20.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.754 | 3.025 | 22.5 | 0.0 | 95.8 | 100.0 | 0.0 | 1.09 | none |
| lang=en | 18 | 1.637 | 2.843 | 21.7 | 0.0 | 100.0 | 100.0 | 0.0 | 1.11 | none |
| lang=hi | 3 | 2.377 | 3.979 | 23.2 | 0.0 | 100.0 | 100.0 | 0.0 | 1.06 | none |
| lang=hinglish | 3 | 2.158 | 3.025 | 26.5 | 0.0 | 66.7 | 100.0 | 0.0 | 1.0 | none |
| set=hard | 24 | 1.754 | 3.025 | 22.5 | 0.0 | 95.8 | 100.0 | 0.0 | 1.09 | none |

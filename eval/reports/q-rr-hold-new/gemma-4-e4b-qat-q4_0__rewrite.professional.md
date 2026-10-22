# rewrite.professional with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:53.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 1.785 | 2.828 | 22.7 | 10.0 | 100.0 | 87.5 | 0.0 | 1.16 | none |
| lang=en | 7 | 1.659 | 2.67 | 20.7 | 0.0 | 100.0 | 100.0 | 0.0 | 1.17 | none |
| lang=hi | 2 | 1.785 | 3.571 | 22.9 | 50.0 | 100.0 | 60.0 | 0.0 | 1.08 | none |
| lang=hinglish | 1 | 2.102 | 2.986 | 23.8 | 0.0 | 100.0 | 0.0 | 0.0 | 1.08 | none |
| set=holdout | 10 | 1.785 | 2.828 | 22.7 | 10.0 | 100.0 | 87.5 | 0.0 | 1.16 | none |

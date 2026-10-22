# rewrite.professional with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 22:48.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 12 | 1.798 | 3.178 | 20.8 | 8.3 | 100.0 | 91.3 | 0.0 | 1.16 | none |
| lang=en | 9 | 1.727 | 3.429 | 19.4 | 0.0 | 100.0 | 100.0 | 0.0 | 1.26 | none |
| lang=hi | 2 | 1.85 | 3.38 | 29.1 | 50.0 | 100.0 | 60.0 | 0.0 | 1.08 | none |
| lang=hinglish | 1 | 2.188 | 2.927 | 28.4 | 0.0 | 100.0 | 0.0 | 0.0 | 1.08 | none |
| set=holdout | 12 | 1.798 | 3.178 | 20.8 | 8.3 | 100.0 | 91.3 | 0.0 | 1.16 | none |

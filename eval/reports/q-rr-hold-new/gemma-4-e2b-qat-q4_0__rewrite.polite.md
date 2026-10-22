# rewrite.polite with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:52.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 0.984 | 1.827 | 34.7 | 10.0 | 100.0 | 87.5 | 10.0 | 1.31 | none |
| lang=en | 7 | 0.913 | 1.669 | 38.8 | 0.0 | 100.0 | 100.0 | 14.3 | 1.33 | none |
| lang=hi | 2 | 1.029 | 2.262 | 32.9 | 50.0 | 100.0 | 60.0 | 0.0 | 1.27 | none |
| lang=hinglish | 1 | 1.222 | 2.007 | 29.3 | 0.0 | 100.0 | 0.0 | 0.0 | 1.25 | none |
| set=holdout | 10 | 0.984 | 1.827 | 34.7 | 10.0 | 100.0 | 87.5 | 10.0 | 1.31 | none |

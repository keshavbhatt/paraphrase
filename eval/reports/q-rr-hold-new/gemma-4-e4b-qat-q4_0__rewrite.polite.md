# rewrite.polite with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:54.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 1.825 | 3.47 | 18.1 | 0.0 | 100.0 | 87.5 | 0.0 | 1.49 | none |
| lang=en | 7 | 1.73 | 3.629 | 18.0 | 0.0 | 100.0 | 100.0 | 0.0 | 1.5 | none |
| lang=hi | 2 | 1.873 | 3.069 | 19.6 | 0.0 | 100.0 | 60.0 | 0.0 | 1.15 | none |
| lang=hinglish | 1 | 2.234 | 3.442 | 18.2 | 0.0 | 100.0 | 0.0 | 0.0 | 1.17 | none |
| set=holdout | 10 | 1.825 | 3.47 | 18.1 | 0.0 | 100.0 | 87.5 | 0.0 | 1.49 | none |

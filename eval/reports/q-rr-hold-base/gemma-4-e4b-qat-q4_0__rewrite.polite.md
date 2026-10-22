# rewrite.polite with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:59.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 1.103 | 2.914 | 19.0 | 10.0 | 100.0 | 87.5 | 0.0 | 1.48 | none |
| lang=en | 7 | 1.031 | 2.734 | 18.8 | 0.0 | 100.0 | 100.0 | 0.0 | 1.67 | none |
| lang=hi | 2 | 1.17 | 2.95 | 22.2 | 50.0 | 100.0 | 60.0 | 0.0 | 1.12 | none |
| lang=hinglish | 1 | 1.552 | 3.093 | 16.9 | 0.0 | 100.0 | 0.0 | 0.0 | 1.25 | none |
| set=holdout | 10 | 1.103 | 2.914 | 19.0 | 10.0 | 100.0 | 87.5 | 0.0 | 1.48 | none |

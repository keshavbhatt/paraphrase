# rewrite.friendly with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 20:00.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 1.12 | 2.791 | 17.2 | 10.0 | 100.0 | 90.6 | 10.0 | 1.49 | none |
| lang=en | 7 | 1.023 | 2.758 | 17.4 | 0.0 | 100.0 | 95.5 | 14.3 | 1.78 | none |
| lang=hi | 2 | 1.16 | 3.583 | 15.2 | 50.0 | 100.0 | 80.0 | 0.0 | 1.15 | none |
| lang=hinglish | 1 | 1.561 | 2.686 | 22.2 | 0.0 | 100.0 | 0.0 | 0.0 | 1.42 | none |
| set=holdout | 10 | 1.12 | 2.791 | 17.2 | 10.0 | 100.0 | 90.6 | 10.0 | 1.49 | none |

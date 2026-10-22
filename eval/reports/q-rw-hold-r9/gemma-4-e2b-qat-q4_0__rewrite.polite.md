# rewrite.polite with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 21:50.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 1.037 | 1.694 | 35.2 | 10.0 | 100.0 | 84.4 | 10.0 | 1.19 | none |
| lang=en | 7 | 0.998 | 1.631 | 35.6 | 0.0 | 100.0 | 95.5 | 14.3 | 1.21 | none |
| lang=hi | 2 | 1.241 | 3.449 | 25.8 | 50.0 | 100.0 | 60.0 | 0.0 | 1.12 | none |
| lang=hinglish | 1 | 1.331 | 2.312 | 21.4 | 0.0 | 100.0 | 0.0 | 0.0 | 1.17 | none |
| set=holdout | 10 | 1.037 | 1.694 | 35.2 | 10.0 | 100.0 | 84.4 | 10.0 | 1.19 | none |

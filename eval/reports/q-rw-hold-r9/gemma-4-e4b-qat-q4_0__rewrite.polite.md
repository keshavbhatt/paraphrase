# rewrite.polite with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 21:49.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 1.928 | 3.142 | 19.9 | 0.0 | 100.0 | 87.5 | 0.0 | 1.17 | none |
| lang=en | 7 | 1.833 | 3.037 | 19.2 | 0.0 | 100.0 | 100.0 | 0.0 | 1.23 | none |
| lang=hi | 2 | 1.944 | 2.907 | 22.3 | 0.0 | 100.0 | 60.0 | 0.0 | 1.04 | none |
| lang=hinglish | 1 | 2.275 | 3.274 | 22.0 | 0.0 | 100.0 | 0.0 | 0.0 | 1.17 | none |
| set=holdout | 10 | 1.928 | 3.142 | 19.9 | 0.0 | 100.0 | 87.5 | 0.0 | 1.17 | none |

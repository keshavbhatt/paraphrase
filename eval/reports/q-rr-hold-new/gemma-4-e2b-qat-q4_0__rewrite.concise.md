# rewrite.concise with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:52.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 0.978 | 1.408 | 43.0 | 10.0 | 80.0 | 100.0 | 0.0 | 0.74 | none |
| lang=en | 7 | 0.929 | 1.362 | 46.2 | 0.0 | 85.7 | 100.0 | 0.0 | 0.68 | none |
| lang=hi | 2 | 1.019 | 1.455 | 40.2 | 0.0 | 50.0 | 100.0 | 0.0 | 0.88 | none |
| lang=hinglish | 1 | 1.204 | 2.432 | 34.2 | 100.0 | 100.0 | 0.0 | 0.0 | 0.75 | none |
| set=holdout | 10 | 0.978 | 1.408 | 43.0 | 10.0 | 80.0 | 100.0 | 0.0 | 0.74 | none |

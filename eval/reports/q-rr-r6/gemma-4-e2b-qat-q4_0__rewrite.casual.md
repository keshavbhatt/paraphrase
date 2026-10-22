# rewrite.casual with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 20:12.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.193 | 1.88 | 39.7 | 4.2 | 70.8 | 96.1 | 4.2 | 1.0 | none |
| lang=en | 18 | 1.14 | 1.866 | 38.1 | 0.0 | 77.8 | 100.0 | 5.6 | 1.0 | none |
| lang=hi | 3 | 1.229 | 2.005 | 42.3 | 33.3 | 66.7 | 78.6 | 0.0 | 1.0 | none |
| lang=hinglish | 3 | 1.367 | 1.822 | 46.1 | 0.0 | 33.3 | 100.0 | 0.0 | 1.0 | none |
| set=hard | 24 | 1.193 | 1.88 | 39.7 | 4.2 | 70.8 | 96.1 | 4.2 | 1.0 | none |

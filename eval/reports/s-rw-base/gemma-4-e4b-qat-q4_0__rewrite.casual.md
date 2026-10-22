# rewrite.casual with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 22:34.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 29 | 1.831 | 3.198 | 21.4 | 0.0 | 93.1 | 92.7 | 0.0 | 1.14 | none |
| lang=en | 22 | 1.711 | 3.272 | 20.1 | 0.0 | 100.0 | 97.0 | 0.0 | 1.18 | none |
| lang=hi | 3 | 1.831 | 2.813 | 24.5 | 0.0 | 100.0 | 57.1 | 0.0 | 1.0 | none |
| lang=hinglish | 4 | 2.552 | 3.595 | 24.9 | 0.0 | 50.0 | 100.0 | 0.0 | 1.0 | none |
| set=hard | 29 | 1.831 | 3.198 | 21.4 | 0.0 | 93.1 | 92.7 | 0.0 | 1.14 | none |

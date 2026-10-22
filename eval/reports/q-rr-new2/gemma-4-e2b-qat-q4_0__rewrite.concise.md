# rewrite.concise with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:33.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.01 | 1.533 | 40.6 | 0.0 | 75.0 | 100.0 | 0.0 | 0.83 | none |
| lang=en | 18 | 0.937 | 1.423 | 39.4 | 0.0 | 83.3 | 100.0 | 0.0 | 0.74 | none |
| lang=hi | 3 | 1.284 | 2.162 | 22.8 | 0.0 | 66.7 | 100.0 | 0.0 | 0.94 | none |
| lang=hinglish | 3 | 1.238 | 1.604 | 50.8 | 0.0 | 33.3 | 100.0 | 0.0 | 1.0 | none |
| set=hard | 24 | 1.01 | 1.533 | 40.6 | 0.0 | 75.0 | 100.0 | 0.0 | 0.83 | none |

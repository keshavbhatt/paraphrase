# rewrite.polite with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 20:13.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.224 | 1.909 | 39.2 | 0.0 | 95.8 | 100.0 | 0.0 | 1.15 | none |
| lang=en | 18 | 1.187 | 1.9 | 39.2 | 0.0 | 100.0 | 100.0 | 0.0 | 1.11 | none |
| lang=hi | 3 | 1.239 | 1.917 | 39.9 | 0.0 | 100.0 | 100.0 | 0.0 | 1.25 | none |
| lang=hinglish | 3 | 1.361 | 2.233 | 33.2 | 0.0 | 66.7 | 100.0 | 0.0 | 1.07 | none |
| set=hard | 24 | 1.224 | 1.909 | 39.2 | 0.0 | 95.8 | 100.0 | 0.0 | 1.15 | none |

# rewrite.friendly with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:48.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.821 | 3.187 | 18.0 | 0.0 | 95.8 | 92.2 | 0.0 | 1.14 | none |
| lang=en | 18 | 1.666 | 3.254 | 17.9 | 0.0 | 100.0 | 100.0 | 0.0 | 1.25 | none |
| lang=hi | 3 | 1.982 | 3.115 | 20.5 | 0.0 | 66.7 | 57.1 | 0.0 | 1.0 | none |
| lang=hinglish | 3 | 2.164 | 3.191 | 22.5 | 0.0 | 100.0 | 100.0 | 0.0 | 1.07 | none |
| set=hard | 24 | 1.821 | 3.187 | 18.0 | 0.0 | 95.8 | 92.2 | 0.0 | 1.14 | none |

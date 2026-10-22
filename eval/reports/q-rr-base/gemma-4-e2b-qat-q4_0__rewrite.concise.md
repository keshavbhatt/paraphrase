# rewrite.concise with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 18:34.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 0.621 | 1.264 | 32.4 | 0.0 | 95.8 | 97.4 | 0.0 | 0.69 | none |
| lang=en | 18 | 0.601 | 1.203 | 30.5 | 0.0 | 94.4 | 96.6 | 0.0 | 0.62 | none |
| lang=hi | 3 | 0.616 | 1.626 | 32.9 | 0.0 | 100.0 | 100.0 | 0.0 | 0.88 | none |
| lang=hinglish | 3 | 0.85 | 1.344 | 47.0 | 0.0 | 100.0 | 100.0 | 0.0 | 0.8 | none |
| set=hard | 24 | 0.621 | 1.264 | 32.4 | 0.0 | 95.8 | 97.4 | 0.0 | 0.69 | none |

# rewrite.concise with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 20:14.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.201 | 1.784 | 37.5 | 0.0 | 91.7 | 98.7 | 0.0 | 0.94 | none |
| lang=en | 18 | 1.177 | 1.7 | 38.0 | 0.0 | 94.4 | 98.3 | 0.0 | 0.91 | none |
| lang=hi | 3 | 1.79 | 3.02 | 34.8 | 0.0 | 100.0 | 100.0 | 0.0 | 1.0 | none |
| lang=hinglish | 3 | 1.397 | 1.904 | 38.7 | 0.0 | 66.7 | 100.0 | 0.0 | 0.93 | none |
| set=hard | 24 | 1.201 | 1.784 | 37.5 | 0.0 | 91.7 | 98.7 | 0.0 | 0.94 | none |

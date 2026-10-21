# rewrite.professional with qwen3.5-0.8b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 01:18.

| group | n | ttft_s_median | total_s_median | gen_tps_median | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.318 | 1.261 | 38.8 | 90.8 | 90.7 | 0.0 | 1.2 | none |
| lang=en | 50 | 0.299 | 1.279 | 38.8 | 96.0 | 89.6 | 0.0 | 1.31 | none |
| lang=hi | 8 | 0.436 | 1.426 | 38.1 | 75.0 | 100.0 | 0.0 | 1.0 | none |
| lang=hinglish | 7 | 0.33 | 0.958 | 41.0 | 71.4 | 93.3 | 0.0 | 1.0 | none |
| set=generated | 65 | 0.318 | 1.261 | 38.8 | 90.8 | 90.7 | 0.0 | 1.2 | none |

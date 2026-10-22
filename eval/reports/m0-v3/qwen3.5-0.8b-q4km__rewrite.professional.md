# rewrite.professional with qwen3.5-0.8b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 03:19.

| group | n | ttft_s_median | total_s_median | gen_tps_median | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.3 | 1.3 | 39.4 | 87.7 | 87.9 | 0.0 | 1.18 | none |
| lang=en | 50 | 0.275 | 1.328 | 39.4 | 94.0 | 88.5 | 0.0 | 1.32 | none |
| lang=hi | 8 | 0.333 | 1.488 | 39.4 | 62.5 | 70.6 | 0.0 | 1.0 | none |
| lang=hinglish | 7 | 0.349 | 1.104 | 40.1 | 71.4 | 100.0 | 0.0 | 1.0 | none |
| set=generated | 65 | 0.3 | 1.3 | 39.4 | 87.7 | 87.9 | 0.0 | 1.18 | none |

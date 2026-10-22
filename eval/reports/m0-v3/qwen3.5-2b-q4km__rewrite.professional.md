# rewrite.professional with qwen3.5-2b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 03:28.

| group | n | ttft_s_median | total_s_median | gen_tps_median | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.637 | 2.277 | 21.5 | 92.3 | 89.7 | 1.5 | 1.16 | rw-hi-014 |
| lang=en | 50 | 0.582 | 2.244 | 21.5 | 98.0 | 89.0 | 0.0 | 1.2 | none |
| lang=hi | 8 | 0.67 | 2.464 | 21.5 | 62.5 | 94.1 | 0.0 | 1.0 | none |
| lang=hinglish | 7 | 0.84 | 2.214 | 21.6 | 85.7 | 93.3 | 14.3 | 1.0 | rw-hi-014 |
| set=generated | 65 | 0.637 | 2.277 | 21.5 | 92.3 | 89.7 | 1.5 | 1.16 | rw-hi-014 |

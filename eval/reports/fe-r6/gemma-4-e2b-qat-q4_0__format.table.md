# format.table with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-26 23:32.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | protected_kept_pct | coverage_median | placeholder_pct | fenced_pct | preamble_pct | added_english_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 9 | 1.632 | 6.224 | 20.0 | 11.1 | 100.0 | 100.0 | 0.6 | 0.0 | 0.0 | 0.0 | 0.0 | 100.0 | fm-hl-101 |
| lang=en | 6 | 1.625 | 6.122 | 20.0 | 0.0 | 100.0 | 100.0 | 0.56 | 0.0 | 0.0 | 0.0 | 0.0 | 100.0 | none |
| lang=hi | 1 | 1.408 | 4.013 | 20.3 | 0.0 | 100.0 | 100.0 | 0.9 | 0.0 | 0.0 | 0.0 | 0.0 |  | none |
| lang=hinglish | 2 | 1.821 | 7.379 | 18.9 | 50.0 | 100.0 | 100.0 | 0.76 | 0.0 | 0.0 | 0.0 | 0.0 |  | fm-hl-101 |
| set=generated | 6 | 1.625 | 6.122 | 19.9 | 0.0 | 100.0 | 100.0 | 0.63 | 0.0 | 0.0 | 0.0 | 0.0 |  | none |
| set=hard | 3 | 1.743 | 6.53 | 20.2 | 33.3 | 100.0 | 100.0 | 0.56 | 0.0 | 0.0 | 0.0 | 0.0 | 100.0 | fm-hl-101 |

# format.table with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-26 23:56.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | protected_kept_pct | coverage_median | placeholder_pct | fenced_pct | preamble_pct | added_english_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 9 | 1.737 | 6.535 | 20.2 | 11.1 | 100.0 | 100.0 | 0.6 | 0.0 | 0.0 | 0.0 | 0.0 | 100.0 | fm-hl-101 |
| lang=en | 6 | 1.7 | 6.594 | 20.4 | 0.0 | 100.0 | 100.0 | 0.56 | 0.0 | 0.0 | 0.0 | 0.0 | 100.0 | none |
| lang=hi | 1 | 1.578 | 4.134 | 20.7 | 0.0 | 100.0 | 100.0 | 0.9 | 0.0 | 0.0 | 0.0 | 0.0 |  | none |
| lang=hinglish | 2 | 2.146 | 8.921 | 19.6 | 50.0 | 100.0 | 100.0 | 0.76 | 0.0 | 0.0 | 0.0 | 0.0 |  | fm-hl-101 |
| set=generated | 6 | 1.7 | 6.149 | 20.4 | 0.0 | 100.0 | 100.0 | 0.6 | 0.0 | 0.0 | 0.0 | 0.0 |  | none |
| set=hard | 3 | 2.256 | 6.724 | 19.0 | 33.3 | 100.0 | 100.0 | 0.56 | 0.0 | 0.0 | 0.0 | 0.0 | 100.0 | fm-hl-101 |

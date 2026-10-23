# format.table with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 4, run 2026-10-23 02:07.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | protected_kept_pct | coverage_median | placeholder_pct | fenced_pct | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 9 | 1.455 | 3.872 | 37.4 | 11.1 | 100.0 | 100.0 | 0.6 | 0.0 | 0.0 | 0.0 | 100.0 | fm-hl-101 |
| lang=en | 6 | 1.363 | 3.82 | 38.0 | 0.0 | 100.0 | 100.0 | 0.56 | 0.0 | 0.0 | 0.0 | 100.0 | none |
| lang=hi | 1 | 1.202 | 2.552 | 39.3 | 0.0 | 100.0 | 100.0 | 0.9 | 0.0 | 0.0 | 0.0 |  | none |
| lang=hinglish | 2 | 1.563 | 4.462 | 34.8 | 50.0 | 100.0 | 100.0 | 0.76 | 0.0 | 0.0 | 0.0 |  | fm-hl-101 |
| set=generated | 6 | 1.461 | 3.95 | 37.2 | 0.0 | 100.0 | 100.0 | 0.63 | 0.0 | 0.0 | 0.0 |  | none |
| set=hard | 3 | 1.257 | 3.872 | 38.6 | 33.3 | 100.0 | 100.0 | 0.56 | 0.0 | 0.0 | 0.0 | 100.0 | fm-hl-101 |

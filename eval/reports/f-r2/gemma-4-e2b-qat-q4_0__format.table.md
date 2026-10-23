# format.table with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 4, run 2026-10-23 01:35.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | protected_kept_pct | coverage_median | placeholder_pct | fenced_pct | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 9 | 1.007 | 3.701 | 36.0 | 11.1 | 100.0 | 100.0 | 0.6 | 0.0 | 0.0 | 0.0 | 100.0 | fm-hl-101 |
| lang=en | 6 | 1.062 | 3.724 | 33.8 | 0.0 | 100.0 | 100.0 | 0.56 | 0.0 | 0.0 | 0.0 | 100.0 | none |
| lang=hi | 1 | 1.007 | 2.402 | 38.0 | 0.0 | 100.0 | 100.0 | 0.9 | 0.0 | 0.0 | 0.0 |  | none |
| lang=hinglish | 2 | 1.249 | 5.987 | 31.3 | 50.0 | 100.0 | 100.0 | 0.71 | 0.0 | 0.0 | 0.0 |  | fm-hl-101 |
| set=generated | 6 | 1.353 | 5.027 | 27.9 | 0.0 | 100.0 | 100.0 | 0.6 | 0.0 | 0.0 | 0.0 |  | none |
| set=hard | 3 | 0.956 | 3.563 | 37.4 | 33.3 | 100.0 | 100.0 | 0.56 | 0.0 | 0.0 | 0.0 | 100.0 | fm-hl-101 |

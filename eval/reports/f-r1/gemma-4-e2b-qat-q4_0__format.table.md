# format.table with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 4, run 2026-10-23 01:09.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | protected_kept_pct | coverage_median | placeholder_pct | fenced_pct | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 9 | 1.094 | 3.38 | 38.4 | 11.1 | 88.9 | 100.0 | 0.53 | 0.0 | 0.0 | 0.0 | 0.0 | fm-hl-101 |
| lang=en | 6 | 1.031 | 3.923 | 38.3 | 0.0 | 100.0 | 100.0 | 0.52 | 0.0 | 0.0 | 0.0 | 0.0 | none |
| lang=hi | 1 | 1.1 | 2.378 | 38.4 | 0.0 | 0.0 | 100.0 | 0.97 | 0.0 | 0.0 | 0.0 |  | none |
| lang=hinglish | 2 | 1.033 | 3.792 | 39.7 | 50.0 | 100.0 | 100.0 | 0.76 | 0.0 | 0.0 | 0.0 |  | fm-hl-101 |
| set=generated | 6 | 1.116 | 3.727 | 37.5 | 0.0 | 83.3 | 100.0 | 0.57 | 0.0 | 0.0 | 0.0 |  | none |
| set=hard | 3 | 0.66 | 3.38 | 43.3 | 33.3 | 100.0 | 100.0 | 0.53 | 0.0 | 0.0 | 0.0 | 0.0 | fm-hl-101 |

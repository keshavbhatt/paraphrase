# format.table with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 4, run 2026-10-23 02:05.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | protected_kept_pct | coverage_median | placeholder_pct | fenced_pct | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 9 | 2.724 | 7.404 | 20.2 | 11.1 | 100.0 | 100.0 | 0.54 | 0.0 | 0.0 | 0.0 | 0.0 | fm-hl-101 |
| lang=en | 6 | 2.645 | 6.977 | 19.5 | 0.0 | 100.0 | 100.0 | 0.51 | 0.0 | 0.0 | 0.0 | 0.0 | none |
| lang=hi | 1 | 2.473 | 4.865 | 20.9 | 0.0 | 100.0 | 100.0 | 0.94 | 0.0 | 0.0 | 0.0 |  | none |
| lang=hinglish | 2 | 2.953 | 8.117 | 21.5 | 50.0 | 100.0 | 100.0 | 0.73 | 0.0 | 0.0 | 0.0 |  | fm-hl-101 |
| set=generated | 6 | 2.645 | 7.523 | 19.6 | 0.0 | 100.0 | 100.0 | 0.55 | 0.0 | 0.0 | 0.0 |  | none |
| set=hard | 3 | 2.827 | 7.404 | 20.5 | 33.3 | 100.0 | 100.0 | 0.5 | 0.0 | 0.0 | 0.0 | 0.0 | fm-hl-101 |

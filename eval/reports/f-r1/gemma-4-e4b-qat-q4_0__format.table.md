# format.table with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 4, run 2026-10-23 01:09.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | protected_kept_pct | coverage_median | placeholder_pct | fenced_pct | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 9 | 2.034 | 7.132 | 19.4 | 22.2 | 100.0 | 100.0 | 0.56 | 0.0 | 0.0 | 0.0 | 0.0 | fm-hl-002, fm-hl-101 |
| lang=en | 6 | 1.952 | 6.255 | 19.4 | 0.0 | 100.0 | 100.0 | 0.51 | 0.0 | 0.0 | 0.0 | 0.0 | none |
| lang=hi | 1 | 11.761 | 35.021 | 2.1 | 0.0 | 100.0 | 100.0 | 0.94 | 0.0 | 0.0 | 0.0 |  | none |
| lang=hinglish | 2 | 9.415 | 31.645 | 15.0 | 100.0 | 100.0 | 100.0 | 0.63 | 0.0 | 0.0 | 0.0 |  | fm-hl-002, fm-hl-101 |
| set=generated | 6 | 2.762 | 6.865 | 18.8 | 16.7 | 100.0 | 100.0 | 0.58 | 0.0 | 0.0 | 0.0 |  | fm-hl-002 |
| set=hard | 3 | 1.905 | 8.656 | 19.4 | 33.3 | 100.0 | 100.0 | 0.51 | 0.0 | 0.0 | 0.0 | 0.0 | fm-hl-101 |

# format.table with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 4, run 2026-10-23 02:28.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | protected_kept_pct | coverage_median | placeholder_pct | fenced_pct | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 9 | 2.563 | 6.182 | 21.3 | 11.1 | 100.0 | 100.0 | 0.54 | 0.0 | 0.0 | 0.0 | 0.0 | fm-hl-101 |
| lang=en | 6 | 2.521 | 6.017 | 21.6 | 0.0 | 100.0 | 100.0 | 0.51 | 0.0 | 0.0 | 0.0 | 0.0 | none |
| lang=hi | 1 | 2.192 | 4.401 | 22.6 | 0.0 | 100.0 | 100.0 | 0.94 | 0.0 | 0.0 | 0.0 |  | none |
| lang=hinglish | 2 | 8.493 | 28.528 | 12.2 | 50.0 | 100.0 | 100.0 | 0.73 | 0.0 | 0.0 | 0.0 |  | fm-hl-101 |
| set=generated | 6 | 2.739 | 6.531 | 21.5 | 0.0 | 100.0 | 100.0 | 0.55 | 0.0 | 0.0 | 0.0 |  | none |
| set=hard | 3 | 2.563 | 5.851 | 21.3 | 33.3 | 100.0 | 100.0 | 0.5 | 0.0 | 0.0 | 0.0 | 0.0 | fm-hl-101 |

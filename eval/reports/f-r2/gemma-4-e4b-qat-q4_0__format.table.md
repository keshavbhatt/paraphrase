# format.table with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 4, run 2026-10-23 01:25.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | protected_kept_pct | coverage_median | placeholder_pct | fenced_pct | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 9 | 6.259 | 18.603 | 8.2 | 11.1 | 100.0 | 100.0 | 0.56 | 0.0 | 0.0 | 0.0 | 0.0 | fm-hl-101 |
| lang=en | 6 | 7.423 | 16.147 | 8.2 | 0.0 | 100.0 | 100.0 | 0.51 | 0.0 | 0.0 | 0.0 | 0.0 | none |
| lang=hi | 1 | 3.62 | 18.603 | 3.3 | 0.0 | 100.0 | 100.0 | 0.94 | 0.0 | 0.0 | 0.0 |  | none |
| lang=hinglish | 2 | 4.37 | 27.328 | 5.5 | 50.0 | 100.0 | 100.0 | 0.71 | 0.0 | 0.0 | 0.0 |  | fm-hl-101 |
| set=generated | 6 | 7.383 | 31.012 | 3.1 | 0.0 | 100.0 | 100.0 | 0.58 | 0.0 | 0.0 | 0.0 |  | none |
| set=hard | 3 | 2.482 | 11.884 | 10.4 | 33.3 | 100.0 | 100.0 | 0.51 | 0.0 | 0.0 | 0.0 | 0.0 | fm-hl-101 |

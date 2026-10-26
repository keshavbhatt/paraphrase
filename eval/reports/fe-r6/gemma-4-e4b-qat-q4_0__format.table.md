# format.table with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-26 23:32.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | protected_kept_pct | coverage_median | placeholder_pct | fenced_pct | preamble_pct | added_english_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 9 | 3.511 | 12.583 | 11.1 | 11.1 | 100.0 | 100.0 | 0.51 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | fm-hl-101 |
| lang=en | 6 | 3.231 | 12.335 | 10.9 | 0.0 | 100.0 | 100.0 | 0.49 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | none |
| lang=hi | 1 | 3.648 | 8.306 | 11.2 | 0.0 | 100.0 | 100.0 | 0.94 | 0.0 | 0.0 | 0.0 | 0.0 |  | none |
| lang=hinglish | 2 | 3.693 | 13.11 | 11.1 | 50.0 | 100.0 | 100.0 | 0.73 | 0.0 | 0.0 | 0.0 | 0.0 |  | fm-hl-101 |
| set=generated | 6 | 3.58 | 12.404 | 11.1 | 0.0 | 100.0 | 100.0 | 0.53 | 0.0 | 0.0 | 0.0 | 0.0 |  | none |
| set=hard | 3 | 2.884 | 12.583 | 11.1 | 33.3 | 100.0 | 100.0 | 0.5 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | fm-hl-101 |

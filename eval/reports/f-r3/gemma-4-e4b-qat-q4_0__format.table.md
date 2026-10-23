# format.table with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 4, run 2026-10-23 01:53.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | protected_kept_pct | coverage_median | placeholder_pct | fenced_pct | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 9 | 1.959 | 6.641 | 21.0 | 11.1 | 100.0 | 100.0 | 0.56 | 0.0 | 0.0 | 0.0 | 0.0 | fm-hl-101 |
| lang=en | 6 | 2.06 | 5.862 | 22.0 | 0.0 | 100.0 | 100.0 | 0.51 | 0.0 | 0.0 | 0.0 | 0.0 | none |
| lang=hi | 1 | 1.959 | 4.69 | 18.3 | 0.0 | 100.0 | 100.0 | 0.94 | 0.0 | 0.0 | 0.0 |  | none |
| lang=hinglish | 2 | 2.204 | 7.6 | 18.1 | 50.0 | 100.0 | 100.0 | 0.71 | 0.0 | 0.0 | 0.0 |  | fm-hl-101 |
| set=generated | 6 | 2.359 | 6.763 | 21.0 | 0.0 | 100.0 | 100.0 | 0.58 | 0.0 | 0.0 | 0.0 |  | none |
| set=hard | 3 | 1.46 | 5.084 | 21.6 | 33.3 | 100.0 | 100.0 | 0.51 | 0.0 | 0.0 | 0.0 | 0.0 | fm-hl-101 |

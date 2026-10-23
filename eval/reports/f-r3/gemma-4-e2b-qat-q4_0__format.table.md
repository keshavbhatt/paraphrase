# format.table with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 4, run 2026-10-23 02:00.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | protected_kept_pct | coverage_median | placeholder_pct | fenced_pct | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 9 | 1.018 | 3.431 | 39.1 | 11.1 | 100.0 | 100.0 | 0.6 | 0.0 | 0.0 | 0.0 | 100.0 | fm-hl-101 |
| lang=en | 6 | 1.039 | 3.339 | 38.7 | 0.0 | 100.0 | 100.0 | 0.56 | 0.0 | 0.0 | 0.0 | 100.0 | none |
| lang=hi | 1 | 1.018 | 2.321 | 40.7 | 0.0 | 100.0 | 100.0 | 0.9 | 0.0 | 0.0 | 0.0 |  | none |
| lang=hinglish | 2 | 1.123 | 4.112 | 37.6 | 50.0 | 100.0 | 100.0 | 0.71 | 0.0 | 0.0 | 0.0 |  | fm-hl-101 |
| set=generated | 6 | 1.194 | 3.696 | 38.1 | 0.0 | 100.0 | 100.0 | 0.63 | 0.0 | 0.0 | 0.0 |  | none |
| set=hard | 3 | 0.765 | 3.431 | 41.6 | 33.3 | 100.0 | 100.0 | 0.56 | 0.0 | 0.0 | 0.0 | 100.0 | fm-hl-101 |

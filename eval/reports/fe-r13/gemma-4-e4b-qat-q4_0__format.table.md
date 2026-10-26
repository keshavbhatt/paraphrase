# format.table with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-27 00:03.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | protected_kept_pct | coverage_median | placeholder_pct | fenced_pct | preamble_pct | added_english_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 9 | 3.63 | 11.622 | 11.4 | 11.1 | 100.0 | 100.0 | 0.56 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | fm-hl-101 |
| lang=en | 6 | 3.234 | 11.517 | 10.8 | 0.0 | 100.0 | 100.0 | 0.53 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | none |
| lang=hi | 1 | 4.745 | 9.831 | 11.4 | 0.0 | 100.0 | 100.0 | 0.97 | 0.0 | 0.0 | 0.0 | 0.0 |  | none |
| lang=hinglish | 2 | 5.055 | 15.013 | 10.8 | 50.0 | 100.0 | 100.0 | 0.76 | 0.0 | 0.0 | 0.0 | 0.0 |  | fm-hl-101 |
| set=generated | 6 | 3.839 | 12.955 | 10.8 | 0.0 | 100.0 | 100.0 | 0.6 | 0.0 | 0.0 | 0.0 | 0.0 |  | none |
| set=hard | 3 | 2.976 | 11.412 | 11.5 | 33.3 | 100.0 | 100.0 | 0.5 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | fm-hl-101 |

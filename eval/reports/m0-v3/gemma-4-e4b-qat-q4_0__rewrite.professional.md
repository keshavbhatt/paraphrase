# rewrite.professional with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 1, run 2026-10-22 04:19.

| group | n | ttft_s_median | total_s_median | gen_tps_median | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 1.439 | 4.214 | 11.9 | 100.0 | 91.1 | 0.0 | 1.2 | rw-hi-005, rw-hi-010 |
| lang=en | 50 | 1.302 | 4.305 | 11.9 | 100.0 | 90.1 | 0.0 | 1.24 | none |
| lang=hi | 8 | 1.628 | 4.48 | 11.9 | 100.0 | 94.1 | 0.0 | 1.16 | rw-hi-005 |
| lang=hinglish | 7 | 1.517 | 3.974 | 12.2 | 100.0 | 100.0 | 0.0 | 1.15 | rw-hi-010 |
| set=generated | 65 | 1.439 | 4.214 | 11.9 | 100.0 | 91.1 | 0.0 | 1.2 | rw-hi-005, rw-hi-010 |

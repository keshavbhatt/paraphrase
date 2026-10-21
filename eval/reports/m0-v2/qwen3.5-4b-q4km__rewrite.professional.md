# rewrite.professional with qwen3.5-4b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 01:59.

| group | n | ttft_s_median | total_s_median | gen_tps_median | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 1.583 | 5.22 | 10.5 | 100.0 | 88.3 | 0.0 | 1.32 | rw-hi-010, rw-hi-011, rw-hi-012, rw-hi-014, rw-hi-015 |
| lang=en | 50 | 1.432 | 5.103 | 10.4 | 100.0 | 90.1 | 0.0 | 1.39 | none |
| lang=hi | 8 | 1.805 | 6.444 | 10.5 | 100.0 | 70.6 | 0.0 | 1.19 | none |
| lang=hinglish | 7 | 1.666 | 5.045 | 10.6 | 100.0 | 86.7 | 0.0 | 1.14 | rw-hi-010, rw-hi-011, rw-hi-012, rw-hi-014, rw-hi-015 |
| set=generated | 65 | 1.583 | 5.22 | 10.5 | 100.0 | 88.3 | 0.0 | 1.32 | rw-hi-010, rw-hi-011, rw-hi-012, rw-hi-014, rw-hi-015 |

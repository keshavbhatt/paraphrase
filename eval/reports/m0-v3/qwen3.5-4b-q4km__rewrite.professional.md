# rewrite.professional with qwen3.5-4b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 04:00.

| group | n | ttft_s_median | total_s_median | gen_tps_median | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 1.65 | 5.198 | 10.6 | 100.0 | 89.3 | 0.0 | 1.38 | rw-hi-010, rw-hi-014, rw-hi-015 |
| lang=en | 50 | 1.437 | 5.182 | 10.5 | 100.0 | 90.7 | 0.0 | 1.41 | none |
| lang=hi | 8 | 2.082 | 6.326 | 10.5 | 100.0 | 70.6 | 0.0 | 1.25 | none |
| lang=hinglish | 7 | 1.878 | 4.276 | 10.7 | 100.0 | 93.3 | 0.0 | 1.24 | rw-hi-010, rw-hi-014, rw-hi-015 |
| set=generated | 65 | 1.65 | 5.198 | 10.6 | 100.0 | 89.3 | 0.0 | 1.38 | rw-hi-010, rw-hi-014, rw-hi-015 |

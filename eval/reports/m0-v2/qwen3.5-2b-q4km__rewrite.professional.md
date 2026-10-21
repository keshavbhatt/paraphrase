# rewrite.professional with qwen3.5-2b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 01:27.

| group | n | ttft_s_median | total_s_median | gen_tps_median | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.663 | 2.377 | 21.6 | 100.0 | 89.3 | 0.0 | 1.15 | rw-hi-010, rw-hi-011, rw-hi-014, rw-hi-015 |
| lang=en | 50 | 0.637 | 2.343 | 21.6 | 100.0 | 89.6 | 0.0 | 1.2 | none |
| lang=hi | 8 | 0.836 | 2.772 | 21.5 | 100.0 | 82.4 | 0.0 | 1.1 | none |
| lang=hinglish | 7 | 0.775 | 2.016 | 21.7 | 100.0 | 93.3 | 0.0 | 1.08 | rw-hi-010, rw-hi-011, rw-hi-014, rw-hi-015 |
| set=generated | 65 | 0.663 | 2.377 | 21.6 | 100.0 | 89.3 | 0.0 | 1.15 | rw-hi-010, rw-hi-011, rw-hi-014, rw-hi-015 |

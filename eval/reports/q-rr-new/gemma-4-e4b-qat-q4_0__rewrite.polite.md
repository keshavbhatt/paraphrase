# rewrite.polite with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:17.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.806 | 3.559 | 17.7 | 12.5 | 100.0 | 100.0 | 0.0 | 1.38 | rw-hi-102 |
| lang=en | 18 | 1.678 | 3.386 | 18.1 | 0.0 | 100.0 | 100.0 | 0.0 | 1.48 | none |
| lang=hi | 3 | 2.007 | 6.933 | 17.1 | 100.0 | 100.0 | 100.0 | 0.0 | 1.18 | rw-hi-102 |
| lang=hinglish | 3 | 2.186 | 3.781 | 17.3 | 0.0 | 100.0 | 100.0 | 0.0 | 1.33 | none |
| set=hard | 24 | 1.806 | 3.559 | 17.7 | 12.5 | 100.0 | 100.0 | 0.0 | 1.38 | rw-hi-102 |

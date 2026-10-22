# rewrite.polite with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:03.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.073 | 2.033 | 30.9 | 8.3 | 100.0 | 100.0 | 0.0 | 1.28 | rw-hi-102 |
| lang=en | 18 | 1.019 | 1.749 | 30.9 | 0.0 | 100.0 | 100.0 | 0.0 | 1.38 | none |
| lang=hi | 3 | 1.384 | 4.134 | 25.9 | 66.7 | 100.0 | 100.0 | 0.0 | 1.17 | rw-hi-102 |
| lang=hinglish | 3 | 1.249 | 2.043 | 31.7 | 0.0 | 100.0 | 100.0 | 0.0 | 1.14 | none |
| set=hard | 24 | 1.073 | 2.033 | 30.9 | 8.3 | 100.0 | 100.0 | 0.0 | 1.28 | rw-hi-102 |

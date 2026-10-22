# rewrite.casual with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 18:32.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 0.682 | 1.556 | 32.3 | 4.2 | 100.0 | 77.9 | 0.0 | 1.1 | none |
| lang=en | 18 | 0.634 | 1.635 | 26.5 | 0.0 | 100.0 | 78.0 | 0.0 | 1.14 | none |
| lang=hi | 3 | 0.737 | 1.526 | 39.3 | 33.3 | 100.0 | 71.4 | 0.0 | 1.0 | none |
| lang=hinglish | 3 | 0.831 | 1.513 | 35.8 | 0.0 | 100.0 | 100.0 | 0.0 | 1.07 | none |
| set=hard | 24 | 0.682 | 1.556 | 32.3 | 4.2 | 100.0 | 77.9 | 0.0 | 1.1 | none |

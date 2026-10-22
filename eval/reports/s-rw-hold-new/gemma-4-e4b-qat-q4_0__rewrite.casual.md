# rewrite.casual with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 22:48.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 12 | 1.812 | 3.35 | 18.7 | 8.3 | 100.0 | 91.3 | 0.0 | 1.16 | none |
| lang=en | 9 | 1.734 | 2.91 | 19.0 | 0.0 | 100.0 | 100.0 | 0.0 | 1.22 | none |
| lang=hi | 2 | 1.953 | 3.914 | 21.8 | 50.0 | 100.0 | 60.0 | 0.0 | 1.04 | none |
| lang=hinglish | 1 | 4.205 | 7.022 | 7.8 | 0.0 | 100.0 | 0.0 | 0.0 | 1.17 | none |
| set=holdout | 12 | 1.812 | 3.35 | 18.7 | 8.3 | 100.0 | 91.3 | 0.0 | 1.16 | none |

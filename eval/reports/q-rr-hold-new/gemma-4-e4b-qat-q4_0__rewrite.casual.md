# rewrite.casual with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:54.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 1.775 | 3.194 | 19.8 | 10.0 | 100.0 | 87.5 | 10.0 | 1.1 | none |
| lang=en | 7 | 1.659 | 3.117 | 19.9 | 0.0 | 100.0 | 100.0 | 14.3 | 1.11 | none |
| lang=hi | 2 | 1.775 | 3.64 | 22.6 | 50.0 | 100.0 | 60.0 | 0.0 | 1.04 | none |
| lang=hinglish | 1 | 2.102 | 3.271 | 18.8 | 0.0 | 100.0 | 0.0 | 0.0 | 1.25 | none |
| set=holdout | 10 | 1.775 | 3.194 | 19.8 | 10.0 | 100.0 | 87.5 | 10.0 | 1.1 | none |

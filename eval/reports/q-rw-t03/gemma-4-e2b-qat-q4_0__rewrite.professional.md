# rewrite.professional with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 21:26.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.194 | 2.02 | 30.3 | 4.2 | 95.8 | 93.5 | 0.0 | 1.09 | none |
| lang=en | 18 | 1.125 | 1.823 | 33.4 | 0.0 | 100.0 | 100.0 | 0.0 | 1.12 | none |
| lang=hi | 3 | 1.23 | 2.213 | 24.8 | 0.0 | 100.0 | 64.3 | 0.0 | 1.08 | none |
| lang=hinglish | 3 | 1.409 | 2.929 | 27.0 | 33.3 | 66.7 | 100.0 | 0.0 | 1.07 | none |
| set=hard | 24 | 1.194 | 2.02 | 30.3 | 4.2 | 95.8 | 93.5 | 0.0 | 1.09 | none |

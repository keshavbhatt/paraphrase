# rewrite.friendly with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 20:31.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 2.326 | 3.746 | 18.4 | 0.0 | 95.8 | 92.2 | 0.0 | 1.13 | none |
| lang=en | 18 | 2.191 | 3.846 | 18.4 | 0.0 | 94.4 | 100.0 | 0.0 | 1.16 | none |
| lang=hi | 3 | 2.413 | 3.857 | 15.9 | 0.0 | 100.0 | 57.1 | 0.0 | 1.0 | none |
| lang=hinglish | 3 | 2.456 | 3.635 | 20.0 | 0.0 | 100.0 | 100.0 | 0.0 | 1.0 | none |
| set=hard | 24 | 2.326 | 3.746 | 18.4 | 0.0 | 95.8 | 92.2 | 0.0 | 1.13 | none |

# rewrite.concise with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 20:29.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 2.307 | 3.77 | 17.0 | 0.0 | 95.8 | 100.0 | 0.0 | 0.88 | none |
| lang=en | 18 | 2.176 | 3.174 | 19.1 | 0.0 | 94.4 | 100.0 | 0.0 | 0.86 | none |
| lang=hi | 3 | 2.611 | 3.898 | 17.2 | 0.0 | 100.0 | 100.0 | 0.0 | 1.0 | none |
| lang=hinglish | 3 | 2.773 | 4.512 | 14.7 | 0.0 | 100.0 | 100.0 | 0.0 | 1.0 | none |
| set=hard | 24 | 2.307 | 3.77 | 17.0 | 0.0 | 95.8 | 100.0 | 0.0 | 0.88 | none |

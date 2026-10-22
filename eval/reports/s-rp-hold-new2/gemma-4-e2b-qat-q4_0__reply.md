# reply with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 23:02.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | valid_pct | distinct_pct | over_15_words_pct | draft_kept_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 12 | 0.993 | 3.138 | 19.5 | 16.7 | 100.0 | 100.0 | 0.0 | 100.0 | 100.0 | rp-hi-201 |
| lang=en | 8 | 0.954 | 3.063 | 19.5 | 0.0 | 100.0 | 100.0 | 0.0 | 100.0 | 100.0 | none |
| lang=hi | 2 | 1.064 | 3.775 | 18.4 | 50.0 | 100.0 | 100.0 | 0.0 |  |  | rp-hi-201 |
| lang=hinglish | 2 | 1.31 | 4.553 | 18.7 | 50.0 | 100.0 | 100.0 | 0.0 | 100.0 |  | none |
| set=holdout | 12 | 0.993 | 3.138 | 19.5 | 16.7 | 100.0 | 100.0 | 0.0 | 100.0 | 100.0 | rp-hi-201 |

# reply with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 22:42.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | valid_pct | distinct_pct | over_15_words_pct | draft_kept_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 12 | 0.515 | 2.739 | 17.9 | 8.3 | 100.0 | 100.0 | 0.0 | 100.0 | 100.0 | rp-hi-201 |
| lang=en | 8 | 0.478 | 2.487 | 18.3 | 0.0 | 100.0 | 100.0 | 0.0 | 100.0 | 100.0 | none |
| lang=hi | 2 | 0.62 | 3.514 | 18.0 | 50.0 | 100.0 | 100.0 | 0.0 |  |  | rp-hi-201 |
| lang=hinglish | 2 | 1.147 | 3.457 | 16.2 | 0.0 | 100.0 | 100.0 | 0.0 | 100.0 |  | none |
| set=holdout | 12 | 0.515 | 2.739 | 17.9 | 8.3 | 100.0 | 100.0 | 0.0 | 100.0 | 100.0 | rp-hi-201 |

# reply with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 23:21.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | valid_pct | distinct_pct | over_15_words_pct | draft_kept_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 13 | 0.466 | 2.461 | 20.3 | 23.1 | 100.0 | 100.0 | 0.0 | 100.0 | 66.7 | rp-hi-201 |
| lang=en | 9 | 0.444 | 2.416 | 20.4 | 0.0 | 100.0 | 100.0 | 0.0 | 100.0 | 66.7 | none |
| lang=hi | 2 | 0.571 | 4.922 | 17.7 | 100.0 | 100.0 | 100.0 | 0.0 |  |  | rp-hi-201 |
| lang=hinglish | 2 | 0.973 | 4.079 | 19.5 | 50.0 | 100.0 | 100.0 | 0.0 | 100.0 |  | none |
| set=holdout | 13 | 0.466 | 2.461 | 20.3 | 23.1 | 100.0 | 100.0 | 0.0 | 100.0 | 66.7 | rp-hi-201 |

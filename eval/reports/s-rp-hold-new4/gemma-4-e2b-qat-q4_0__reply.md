# reply with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 23:14.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | valid_pct | distinct_pct | over_15_words_pct | draft_kept_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 13 | 0.924 | 2.869 | 20.2 | 15.4 | 100.0 | 100.0 | 0.0 | 100.0 | 100.0 | rp-hi-201, rp-hi-202 |
| lang=en | 9 | 0.865 | 2.749 | 20.4 | 0.0 | 100.0 | 100.0 | 0.0 | 100.0 | 100.0 | none |
| lang=hi | 2 | 1.034 | 3.812 | 19.8 | 50.0 | 100.0 | 100.0 | 0.0 |  |  | rp-hi-201 |
| lang=hinglish | 2 | 1.618 | 4.864 | 19.9 | 50.0 | 100.0 | 100.0 | 0.0 | 100.0 |  | rp-hi-202 |
| set=holdout | 13 | 0.924 | 2.869 | 20.2 | 15.4 | 100.0 | 100.0 | 0.0 | 100.0 | 100.0 | rp-hi-201, rp-hi-202 |

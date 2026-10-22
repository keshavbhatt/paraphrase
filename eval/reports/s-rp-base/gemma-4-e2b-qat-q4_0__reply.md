# reply with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 22:29.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | valid_pct | distinct_pct | over_15_words_pct | draft_kept_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 31 | 0.555 | 2.454 | 19.6 | 16.1 | 100.0 | 96.8 | 0.0 | 100.0 | 60.0 | rp-hi-105, rp-hi-109 |
| lang=en | 22 | 0.489 | 2.363 | 19.3 | 0.0 | 100.0 | 95.5 | 0.0 | 100.0 | 50.0 | none |
| lang=hi | 4 | 0.574 | 2.783 | 19.8 | 25.0 | 100.0 | 100.0 | 0.0 |  |  | none |
| lang=hinglish | 5 | 0.869 | 4.438 | 20.0 | 80.0 | 100.0 | 100.0 | 0.0 |  | 100.0 | rp-hi-105, rp-hi-109 |
| set=hard | 31 | 0.555 | 2.454 | 19.6 | 16.1 | 100.0 | 96.8 | 0.0 | 100.0 | 60.0 | rp-hi-105, rp-hi-109 |

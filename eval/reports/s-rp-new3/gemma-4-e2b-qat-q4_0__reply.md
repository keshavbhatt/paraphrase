# reply with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 23:04.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | valid_pct | distinct_pct | over_15_words_pct | draft_kept_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 31 | 1.097 | 3.056 | 17.9 | 9.7 | 100.0 | 96.8 | 0.0 | 100.0 | 40.0 | rp-hi-105, rp-hi-109 |
| lang=en | 22 | 1.029 | 2.877 | 18.6 | 0.0 | 100.0 | 95.5 | 0.0 | 100.0 | 25.0 | none |
| lang=hi | 4 | 1.262 | 3.882 | 17.0 | 25.0 | 100.0 | 100.0 | 0.0 |  |  | none |
| lang=hinglish | 5 | 1.345 | 4.223 | 15.7 | 40.0 | 100.0 | 100.0 | 0.0 |  | 100.0 | rp-hi-105, rp-hi-109 |
| set=hard | 31 | 1.097 | 3.056 | 17.9 | 9.7 | 100.0 | 96.8 | 0.0 | 100.0 | 40.0 | rp-hi-105, rp-hi-109 |

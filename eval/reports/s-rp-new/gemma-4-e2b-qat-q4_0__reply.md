# reply with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 22:41.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | valid_pct | distinct_pct | over_15_words_pct | draft_kept_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 31 | 0.605 | 2.427 | 19.0 | 9.7 | 100.0 | 96.8 | 0.0 | 100.0 | 40.0 | rp-hi-105, rp-hi-109 |
| lang=en | 22 | 0.507 | 2.392 | 19.3 | 0.0 | 100.0 | 95.5 | 0.0 | 100.0 | 25.0 | none |
| lang=hi | 4 | 0.649 | 3.108 | 18.9 | 25.0 | 100.0 | 100.0 | 0.0 |  |  | none |
| lang=hinglish | 5 | 0.935 | 3.874 | 18.8 | 40.0 | 100.0 | 100.0 | 0.0 |  | 100.0 | rp-hi-105, rp-hi-109 |
| set=hard | 31 | 0.605 | 2.427 | 19.0 | 9.7 | 100.0 | 96.8 | 0.0 | 100.0 | 40.0 | rp-hi-105, rp-hi-109 |

# reply with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 23:39.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | valid_pct | distinct_pct | over_15_words_pct | draft_kept_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 35 | 0.511 | 2.218 | 20.3 | 8.6 | 100.0 | 97.1 | 0.0 | 100.0 | 44.4 | rp-hi-105, rp-hi-109 |
| lang=en | 25 | 0.459 | 2.157 | 20.5 | 0.0 | 100.0 | 96.0 | 0.0 | 100.0 | 42.9 | none |
| lang=hi | 4 | 0.714 | 2.825 | 19.6 | 25.0 | 100.0 | 100.0 | 0.0 |  |  | none |
| lang=hinglish | 6 | 0.863 | 2.955 | 20.1 | 33.3 | 100.0 | 100.0 | 0.0 |  | 50.0 | rp-hi-105, rp-hi-109 |
| set=hard | 35 | 0.511 | 2.218 | 20.3 | 8.6 | 100.0 | 97.1 | 0.0 | 100.0 | 44.4 | rp-hi-105, rp-hi-109 |

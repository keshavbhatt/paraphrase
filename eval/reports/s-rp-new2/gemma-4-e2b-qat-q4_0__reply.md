# reply with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 23:01.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | valid_pct | distinct_pct | over_15_words_pct | draft_kept_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 31 | 1.052 | 3.169 | 19.0 | 12.9 | 100.0 | 96.8 | 0.0 | 100.0 | 20.0 | rp-hi-105, rp-hi-109 |
| lang=en | 22 | 0.976 | 3.006 | 18.9 | 0.0 | 100.0 | 95.5 | 0.0 | 100.0 | 0.0 | none |
| lang=hi | 4 | 1.078 | 4.046 | 19.5 | 50.0 | 100.0 | 100.0 | 0.0 |  |  | none |
| lang=hinglish | 5 | 1.187 | 3.77 | 18.3 | 40.0 | 100.0 | 100.0 | 0.0 |  | 100.0 | rp-hi-105, rp-hi-109 |
| set=hard | 31 | 1.052 | 3.169 | 19.0 | 12.9 | 100.0 | 96.8 | 0.0 | 100.0 | 20.0 | rp-hi-105, rp-hi-109 |

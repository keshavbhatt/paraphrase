# reply with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 23:20.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | valid_pct | distinct_pct | over_15_words_pct | draft_kept_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 35 | 0.502 | 2.387 | 20.3 | 11.4 | 100.0 | 97.1 | 0.0 | 100.0 | 22.2 | rp-hi-105, rp-hi-109 |
| lang=en | 25 | 0.443 | 2.198 | 20.3 | 0.0 | 100.0 | 96.0 | 0.0 | 100.0 | 14.3 | none |
| lang=hi | 4 | 0.567 | 3.079 | 20.6 | 50.0 | 100.0 | 100.0 | 0.0 |  |  | none |
| lang=hinglish | 6 | 0.846 | 3.24 | 20.3 | 33.3 | 100.0 | 100.0 | 0.0 |  | 50.0 | rp-hi-105, rp-hi-109 |
| set=hard | 35 | 0.502 | 2.387 | 20.3 | 11.4 | 100.0 | 97.1 | 0.0 | 100.0 | 22.2 | rp-hi-105, rp-hi-109 |

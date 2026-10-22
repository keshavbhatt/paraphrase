# reply with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 23:13.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | valid_pct | distinct_pct | over_15_words_pct | draft_kept_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 35 | 0.96 | 2.693 | 20.2 | 11.4 | 100.0 | 94.3 | 0.0 | 100.0 | 22.2 | rp-hi-108, rp-hi-109 |
| lang=en | 25 | 0.933 | 2.644 | 20.3 | 0.0 | 100.0 | 96.0 | 0.0 | 100.0 | 14.3 | none |
| lang=hi | 4 | 1.197 | 3.997 | 18.6 | 25.0 | 100.0 | 100.0 | 0.0 |  |  | rp-hi-108 |
| lang=hinglish | 6 | 1.115 | 3.654 | 20.1 | 50.0 | 100.0 | 83.3 | 0.0 |  | 50.0 | rp-hi-109 |
| set=hard | 35 | 0.96 | 2.693 | 20.2 | 11.4 | 100.0 | 94.3 | 0.0 | 100.0 | 22.2 | rp-hi-108, rp-hi-109 |

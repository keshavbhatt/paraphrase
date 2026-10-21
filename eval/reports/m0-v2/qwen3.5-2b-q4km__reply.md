# reply with qwen3.5-2b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 01:23.

| group | n | ttft_s_median | total_s_median | gen_tps_median | valid_pct | distinct_pct | over_15_words_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.595 | 2.616 | 21.4 | 100.0 | 98.5 | 4.6 | rp-hi-015 |
| lang=en | 50 | 0.582 | 2.463 | 21.5 | 100.0 | 100.0 | 4.0 | none |
| lang=hi | 8 | 0.668 | 3.711 | 21.4 | 100.0 | 87.5 | 12.5 | none |
| lang=hinglish | 7 | 0.733 | 2.745 | 21.3 | 100.0 | 100.0 | 0.0 | rp-hi-015 |
| set=generated | 65 | 0.595 | 2.616 | 21.4 | 100.0 | 98.5 | 4.6 | rp-hi-015 |

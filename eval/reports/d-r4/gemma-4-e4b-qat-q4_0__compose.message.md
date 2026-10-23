# compose.message with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-23 09:04.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | facts_kept_pct | words_median | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 6 | 1.44 | 4.955 | 10.3 | 16.7 | 100.0 | 60.0 | 23.0 | 0.0 | 16.7 | co-hi-101 |
| lang=en | 4 | 1.06 | 4.644 | 10.6 | 0.0 | 100.0 | 33.3 | 25.0 | 0.0 | 0.0 | none |
| lang=hi | 1 | 3.307 | 19.905 | 3.2 | 100.0 | 100.0 | 0.0 | 24 | 0.0 | 100.0 | co-hi-101 |
| lang=hinglish | 1 | 1.984 | 4.481 | 9.2 | 0.0 | 100.0 | 100.0 | 17 | 0.0 | 0.0 | none |
| set=generated | 3 | 1.742 | 4.481 | 10.6 | 0.0 | 100.0 | 75.0 | 22 | 0.0 | 0.0 | none |
| set=hard | 3 | 0.982 | 5.714 | 10.1 | 33.3 | 100.0 | 0.0 | 24 | 0.0 | 33.3 | co-hi-101 |

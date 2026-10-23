# compose.message with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-23 21:16.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | facts_kept_pct | words_median | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 7 | 1.485 | 5.464 | 9.7 | 14.3 | 100.0 | 100.0 | 17 | 0.0 | 14.3 | co-hi-101 |
| lang=en | 5 | 1.197 | 4.531 | 9.7 | 0.0 | 100.0 | 100.0 | 26 | 0.0 | 0.0 | none |
| lang=hi | 1 | 2.23 | 7.839 | 9.6 | 100.0 | 100.0 | 0.0 | 17 | 0.0 | 100.0 | co-hi-101 |
| lang=hinglish | 1 | 4.602 | 11.055 | 3.6 | 0.0 | 100.0 | 100.0 | 17 | 0.0 | 0.0 | none |
| set=generated | 3 | 3.396 | 7.33 | 9.7 | 0.0 | 100.0 | 100.0 | 26 | 0.0 | 0.0 | none |
| set=hard | 4 | 1.18 | 4.16 | 9.6 | 25.0 | 100.0 | 100.0 | 14.0 | 0.0 | 25.0 | co-hi-101 |

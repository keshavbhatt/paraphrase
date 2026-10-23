# compose.message with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-23 18:00.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | facts_kept_pct | words_median | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 7 | 1.444 | 3.99 | 16.3 | 14.3 | 100.0 | 100.0 | 22 | 0.0 | 28.6 | co-hi-101 |
| lang=en | 5 | 1.444 | 3.99 | 16.3 | 0.0 | 100.0 | 100.0 | 32 | 0.0 | 20.0 | none |
| lang=hi | 1 | 1.438 | 4.964 | 17.1 | 100.0 | 100.0 | 0.0 | 22 | 0.0 | 100.0 | co-hi-101 |
| lang=hinglish | 1 | 1.479 | 2.79 | 16.0 | 0.0 | 100.0 | 100.0 | 14 | 0.0 | 0.0 | none |
| set=generated | 3 | 1.479 | 3.99 | 16.1 | 0.0 | 100.0 | 100.0 | 32 | 0.0 | 0.0 | none |
| set=hard | 4 | 1.333 | 3.862 | 17.2 | 25.0 | 100.0 | 100.0 | 20.5 | 0.0 | 50.0 | co-hi-101 |

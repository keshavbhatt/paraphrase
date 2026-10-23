# compose.message with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-23 10:32.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | facts_kept_pct | words_median | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 6 | 0.771 | 2.99 | 20.3 | 16.7 | 100.0 | 100.0 | 24.0 | 0.0 | 16.7 | none |
| lang=en | 4 | 0.58 | 2.99 | 20.3 | 0.0 | 100.0 | 100.0 | 32.5 | 0.0 | 25.0 | none |
| lang=hi | 1 | 0.907 | 4.059 | 21.8 | 100.0 | 100.0 | 0.0 | 15 | 0.0 | 0.0 | none |
| lang=hinglish | 1 | 1.014 | 2.139 | 20.4 | 0.0 | 100.0 | 100.0 | 17 | 0.0 | 0.0 | none |
| set=generated | 3 | 1.014 | 2.563 | 20.3 | 0.0 | 100.0 | 100.0 | 30 | 0.0 | 0.0 | none |
| set=hard | 3 | 0.524 | 4.059 | 20.8 | 33.3 | 100.0 | 0.0 | 18 | 0.0 | 33.3 | none |

# compose.message with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-23 11:49.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | facts_kept_pct | words_median | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 6 | 1.109 | 3.358 | 19.3 | 16.7 | 100.0 | 100.0 | 25.5 | 0.0 | 16.7 | none |
| lang=en | 4 | 1.032 | 3.358 | 19.3 | 0.0 | 100.0 | 100.0 | 30.0 | 0.0 | 25.0 | none |
| lang=hi | 1 | 1.13 | 5.863 | 19.7 | 100.0 | 100.0 | 0.0 | 18 | 0.0 | 0.0 | none |
| lang=hinglish | 1 | 1.229 | 2.42 | 19.3 | 0.0 | 100.0 | 100.0 | 17 | 0.0 | 0.0 | none |
| set=generated | 3 | 1.229 | 3.322 | 19.3 | 0.0 | 100.0 | 100.0 | 28 | 0.0 | 0.0 | none |
| set=hard | 3 | 0.975 | 4.836 | 19.5 | 33.3 | 100.0 | 0.0 | 23 | 0.0 | 33.3 | none |

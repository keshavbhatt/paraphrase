# compose.message with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-23 20:51.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | facts_kept_pct | words_median | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 7 | 1.183 | 3.676 | 17.5 | 14.3 | 85.7 | 100.0 | 22 | 0.0 | 28.6 | co-hi-101 |
| lang=en | 5 | 1.162 | 3.676 | 16.4 | 0.0 | 80.0 | 100.0 | 28 | 0.0 | 20.0 | none |
| lang=hi | 1 | 1.183 | 4.617 | 17.5 | 100.0 | 100.0 | 0.0 | 22 | 0.0 | 100.0 | co-hi-101 |
| lang=hinglish | 1 | 1.195 | 2.36 | 18.0 | 0.0 | 100.0 | 100.0 | 14 | 0.0 | 0.0 | none |
| set=generated | 3 | 1.195 | 3.676 | 16.4 | 0.0 | 100.0 | 100.0 | 28 | 0.0 | 0.0 | none |
| set=hard | 4 | 1.103 | 3.561 | 17.5 | 25.0 | 75.0 | 100.0 | 21.0 | 0.0 | 50.0 | co-hi-101 |

# compose.message with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-23 17:48.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | facts_kept_pct | words_median | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 4 | 2.024 | 5.916 | 10.1 | 25.0 | 100.0 | 100.0 | 24.0 | 0.0 | 25.0 | co-hi-101 |
| lang=en | 3 | 1.879 | 4.043 | 10.2 | 0.0 | 100.0 | 100.0 | 16 | 0.0 | 0.0 | none |
| lang=hi | 1 | 2.323 | 12.076 | 9.7 | 100.0 | 100.0 | 0.0 | 32 | 0.0 | 100.0 | co-hi-101 |
| set=hard | 4 | 2.024 | 5.916 | 10.1 | 25.0 | 100.0 | 100.0 | 24.0 | 0.0 | 25.0 | co-hi-101 |

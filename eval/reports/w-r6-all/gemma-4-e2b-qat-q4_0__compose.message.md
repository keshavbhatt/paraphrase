# compose.message with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-23 21:18.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | facts_kept_pct | words_median | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 7 | 0.826 | 3.109 | 17.6 | 14.3 | 85.7 | 100.0 | 23 | 0.0 | 14.3 | none |
| lang=en | 5 | 0.624 | 3.109 | 17.1 | 0.0 | 80.0 | 100.0 | 30 | 0.0 | 20.0 | none |
| lang=hi | 1 | 1.08 | 3.538 | 19.1 | 100.0 | 100.0 | 0.0 | 10 | 0.0 | 0.0 | none |
| lang=hinglish | 1 | 1.11 | 2.403 | 17.8 | 0.0 | 100.0 | 100.0 | 17 | 0.0 | 0.0 | none |
| set=generated | 3 | 1.069 | 3.109 | 17.6 | 0.0 | 100.0 | 100.0 | 30 | 0.0 | 0.0 | none |
| set=hard | 4 | 0.584 | 2.943 | 17.4 | 25.0 | 75.0 | 100.0 | 17.0 | 0.0 | 25.0 | none |

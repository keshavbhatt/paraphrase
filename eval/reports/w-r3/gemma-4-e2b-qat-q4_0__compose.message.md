# compose.message with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-23 10:37.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | facts_kept_pct | words_median | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 6 | 0.92 | 3.135 | 20.5 | 16.7 | 100.0 | 100.0 | 27.5 | 0.0 | 16.7 | none |
| lang=en | 4 | 0.726 | 3.135 | 20.3 | 0.0 | 100.0 | 100.0 | 33.0 | 0.0 | 25.0 | none |
| lang=hi | 1 | 0.993 | 4.667 | 21.0 | 100.0 | 100.0 | 0.0 | 14 | 0.0 | 0.0 | none |
| lang=hinglish | 1 | 1.099 | 2.204 | 20.8 | 0.0 | 100.0 | 100.0 | 17 | 0.0 | 0.0 | none |
| set=generated | 3 | 1.095 | 2.804 | 20.4 | 0.0 | 100.0 | 100.0 | 33 | 0.0 | 0.0 | none |
| set=hard | 3 | 0.605 | 4.661 | 20.5 | 33.3 | 100.0 | 0.0 | 22 | 0.0 | 33.3 | none |

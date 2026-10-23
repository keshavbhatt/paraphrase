# compose.message with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-23 09:13.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | facts_kept_pct | words_median | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 6 | 0.801 | 5.18 | 13.4 | 16.7 | 100.0 | 100.0 | 26.0 | 0.0 | 16.7 | none |
| lang=en | 4 | 0.56 | 3.782 | 16.1 | 0.0 | 100.0 | 100.0 | 35.5 | 0.0 | 25.0 | none |
| lang=hi | 1 | 4.898 | 19.246 | 3.5 | 100.0 | 100.0 | 0.0 | 20 | 0.0 | 0.0 | none |
| lang=hinglish | 1 | 1.174 | 5.466 | 4.9 | 0.0 | 100.0 | 100.0 | 14 | 0.0 | 0.0 | none |
| set=generated | 3 | 1.04 | 4.895 | 14.5 | 0.0 | 100.0 | 100.0 | 32 | 0.0 | 0.0 | none |
| set=hard | 3 | 0.563 | 5.482 | 12.3 | 33.3 | 100.0 | 100.0 | 20 | 0.0 | 33.3 | none |

# custom with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-23 09:02.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | facts_kept_pct | words_median | placeholder_pct | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 22 | 2.417 | 8.8 | 6.6 | 13.6 | 78.9 | 29.5 | 13.6 | 0.0 | 18.2 | cu-hl-101 |
| lang=en | 14 | 2.193 | 7.895 | 8.6 | 0.0 | 77.3 | 30.0 | 14.3 | 0.0 | 21.4 | none |
| lang=hi | 4 | 2.581 | 17.535 | 5.7 | 50.0 | 66.7 | 34.0 | 25.0 | 0.0 | 25.0 | none |
| lang=hinglish | 4 | 2.936 | 8.186 | 5.9 | 25.0 | 90.0 | 16.5 | 0.0 | 0.0 | 0.0 | cu-hl-101 |
| set=generated | 13 | 2.668 | 7.354 | 5.9 | 7.7 | 81.6 | 30 | 7.7 | 0.0 | 7.7 | none |
| set=hard | 9 | 2.229 | 9.017 | 6.7 | 22.2 | 73.7 | 27 | 22.2 | 0.0 | 33.3 | cu-hl-101 |

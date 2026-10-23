# custom with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-23 09:12.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | facts_kept_pct | words_median | placeholder_pct | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 22 | 1.487 | 7.071 | 7.5 | 13.6 | 82.5 | 25.5 | 18.2 | 0.0 | 27.3 | cu-hl-101 |
| lang=en | 14 | 1.41 | 7.071 | 6.7 | 0.0 | 79.5 | 30.5 | 14.3 | 0.0 | 28.6 | none |
| lang=hi | 4 | 1.515 | 7.217 | 9.3 | 50.0 | 66.7 | 27.5 | 25.0 | 0.0 | 25.0 | none |
| lang=hinglish | 4 | 1.721 | 6.88 | 8.2 | 25.0 | 100.0 | 21.0 | 25.0 | 0.0 | 25.0 | cu-hl-101 |
| set=generated | 13 | 1.641 | 6.724 | 7.1 | 7.7 | 86.8 | 30 | 15.4 | 0.0 | 15.4 | none |
| set=hard | 9 | 1.29 | 7.418 | 7.9 | 22.2 | 73.7 | 24 | 22.2 | 0.0 | 44.4 | cu-hl-101 |

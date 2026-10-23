# custom with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-23 09:47.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | facts_kept_pct | words_median | placeholder_pct | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 22 | 2.196 | 5.913 | 10.9 | 13.6 | 80.7 | 30.0 | 4.5 | 0.0 | 9.1 | cu-hl-101 |
| lang=en | 14 | 2.01 | 5.792 | 10.9 | 0.0 | 79.5 | 30.0 | 0.0 | 0.0 | 7.1 | none |
| lang=hi | 4 | 2.259 | 8.49 | 10.6 | 50.0 | 66.7 | 35.0 | 25.0 | 0.0 | 25.0 | none |
| lang=hinglish | 4 | 2.597 | 5.742 | 11.1 | 25.0 | 90.0 | 17.0 | 0.0 | 0.0 | 0.0 | cu-hl-101 |
| set=generated | 13 | 2.324 | 6.085 | 10.9 | 7.7 | 81.6 | 30 | 0.0 | 0.0 | 0.0 | none |
| set=hard | 9 | 1.923 | 5.582 | 10.9 | 22.2 | 78.9 | 28 | 11.1 | 0.0 | 22.2 | cu-hl-101 |

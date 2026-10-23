# draft.email with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-23 09:22.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | facts_kept_pct | words_median | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 18 | 1.363 | 3.883 | 20.0 | 5.6 | 100.0 | 88.2 | 30.5 | 0.0 | 27.8 | dr-hl-102 |
| lang=en | 13 | 1.237 | 3.403 | 20.0 | 0.0 | 100.0 | 100.0 | 30 | 0.0 | 38.5 | none |
| lang=hi | 3 | 1.508 | 4.015 | 20.0 | 0.0 | 100.0 | 50.0 | 33 | 0.0 | 0.0 | none |
| lang=hinglish | 2 | 1.639 | 4.566 | 20.6 | 50.0 | 100.0 | 100.0 | 20.0 | 0.0 | 0.0 | dr-hl-102 |
| set=generated | 7 | 1.546 | 4.015 | 20.1 | 0.0 | 100.0 | 66.7 | 31 | 0.0 | 14.3 | none |
| set=hard | 11 | 1.237 | 3.306 | 20.0 | 9.1 | 100.0 | 100.0 | 30 | 0.0 | 36.4 | dr-hl-102 |

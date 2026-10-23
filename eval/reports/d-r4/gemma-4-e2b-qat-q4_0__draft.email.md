# draft.email with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-23 09:08.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | facts_kept_pct | words_median | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 12 | 2.287 | 9.624 | 7.1 | 8.3 | 100.0 | 55.6 | 29.0 | 0.0 | 33.3 | dr-hl-102 |
| lang=en | 8 | 2.692 | 13.209 | 5.3 | 0.0 | 100.0 | 60.0 | 24.5 | 0.0 | 37.5 | none |
| lang=hi | 2 | 2.527 | 9.624 | 8.0 | 0.0 | 100.0 | 33.3 | 36.5 | 0.0 | 50.0 | none |
| lang=hinglish | 2 | 1.891 | 5.501 | 13.4 | 50.0 | 100.0 | 100.0 | 20.0 | 0.0 | 0.0 | dr-hl-102 |
| set=generated | 7 | 2.83 | 13.309 | 6.4 | 0.0 | 100.0 | 66.7 | 36 | 0.0 | 42.9 | none |
| set=hard | 5 | 1.896 | 6.459 | 9.3 | 20.0 | 100.0 | 33.3 | 20 | 0.0 | 20.0 | dr-hl-102 |

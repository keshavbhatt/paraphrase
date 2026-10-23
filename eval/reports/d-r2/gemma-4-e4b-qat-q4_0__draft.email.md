# draft.email with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 4, run 2026-10-23 02:11.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | facts_kept_pct | words_median | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 12 | 3.461 | 8.349 | 10.0 | 8.3 | 91.7 | 66.7 | 21.5 | 0.0 | 8.3 | dr-hl-102 |
| lang=en | 8 | 2.752 | 6.337 | 10.7 | 0.0 | 87.5 | 60.0 | 21.5 | 0.0 | 12.5 | none |
| lang=hi | 2 | 6.828 | 14.847 | 5.8 | 0.0 | 100.0 | 66.7 | 18.0 | 0.0 | 0.0 | none |
| lang=hinglish | 2 | 7.124 | 18.817 | 4.6 | 50.0 | 100.0 | 100.0 | 23.0 | 0.0 | 0.0 | dr-hl-102 |
| set=generated | 7 | 5.704 | 15.183 | 9.6 | 0.0 | 100.0 | 83.3 | 24 | 0.0 | 14.3 | none |
| set=hard | 5 | 2.045 | 4.574 | 11.1 | 20.0 | 80.0 | 33.3 | 17 | 0.0 | 0.0 | dr-hl-102 |

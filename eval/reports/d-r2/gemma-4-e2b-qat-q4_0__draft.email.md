# draft.email with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 4, run 2026-10-23 02:13.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | facts_kept_pct | words_median | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 12 | 1.385 | 3.716 | 19.4 | 8.3 | 100.0 | 77.8 | 31.5 | 0.0 | 8.3 | dr-hl-102 |
| lang=en | 8 | 1.303 | 3.903 | 18.2 | 0.0 | 100.0 | 100.0 | 33.0 | 0.0 | 12.5 | none |
| lang=hi | 2 | 1.416 | 3.81 | 19.6 | 0.0 | 100.0 | 33.3 | 32.0 | 0.0 | 0.0 | none |
| lang=hinglish | 2 | 1.519 | 3.345 | 20.2 | 50.0 | 100.0 | 100.0 | 14.5 | 0.0 | 0.0 | dr-hl-102 |
| set=generated | 7 | 1.412 | 4.106 | 17.9 | 0.0 | 100.0 | 66.7 | 32 | 0.0 | 14.3 | none |
| set=hard | 5 | 1.184 | 2.887 | 20.0 | 20.0 | 100.0 | 100.0 | 21 | 0.0 | 0.0 | dr-hl-102 |

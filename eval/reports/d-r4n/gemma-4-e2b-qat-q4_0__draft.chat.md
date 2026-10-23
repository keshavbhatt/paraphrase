# draft.chat with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-23 09:22.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | shape_ok_pct | facts_kept_pct | words_median | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 13 | 1.23 | 1.943 | 20.0 | 7.7 | 100.0 | 100.0 | 8 | 7.7 | 7.7 | dr-hi-105 |
| lang=en | 7 | 1.047 | 1.848 | 19.7 | 0.0 | 100.0 | 100.0 | 14 | 14.3 | 0.0 | none |
| lang=hi | 3 | 1.287 | 2.587 | 19.7 | 33.3 | 100.0 | 0.0 | 7 | 0.0 | 33.3 | dr-hi-105 |
| lang=hinglish | 3 | 1.435 | 1.943 | 21.3 | 0.0 | 100.0 | 100.0 | 8 | 0.0 | 0.0 | none |
| set=generated | 7 | 1.274 | 2.068 | 20.0 | 0.0 | 100.0 | 100.0 | 9 | 0.0 | 0.0 | none |
| set=hard | 6 | 1.107 | 1.903 | 20.1 | 16.7 | 100.0 | 100.0 | 7.0 | 16.7 | 16.7 | dr-hi-105 |

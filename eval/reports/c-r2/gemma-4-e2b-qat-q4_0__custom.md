# custom with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 5, run 2026-10-23 09:49.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | facts_kept_pct | words_median | placeholder_pct | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 22 | 1.206 | 3.327 | 19.7 | 13.6 | 78.9 | 25.5 | 4.5 | 0.0 | 13.6 | cu-hl-101 |
| lang=en | 14 | 1.199 | 3.294 | 19.2 | 0.0 | 75.0 | 29.5 | 0.0 | 0.0 | 14.3 | none |
| lang=hi | 4 | 1.182 | 3.774 | 20.2 | 50.0 | 66.7 | 27.0 | 0.0 | 0.0 | 0.0 | none |
| lang=hinglish | 4 | 1.367 | 3.389 | 20.4 | 25.0 | 100.0 | 20.5 | 25.0 | 0.0 | 25.0 | cu-hl-101 |
| set=generated | 13 | 1.296 | 3.596 | 19.8 | 7.7 | 84.2 | 30 | 7.7 | 0.0 | 7.7 | none |
| set=hard | 9 | 1.182 | 3.117 | 18.6 | 22.2 | 68.4 | 24 | 0.0 | 0.0 | 22.2 | cu-hl-101 |

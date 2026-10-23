# summarize with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 4, run 2026-10-23 01:09.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | facts_kept_pct | ratio_median | longer_pct | bullets_pct | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 28 | 1.435 | 6.16 | 18.4 | 0.0 | 92.7 | 0.82 | 7.1 | 96.4 | 0.0 | 14.3 | none |
| lang=en | 18 | 1.375 | 5.887 | 19.2 | 0.0 | 87.9 | 0.71 | 5.6 | 94.4 | 0.0 | 25.0 | none |
| lang=hi | 5 | 1.247 | 5.415 | 17.9 | 0.0 | 100.0 | 0.96 | 0.0 | 100.0 | 0.0 | 0.0 | none |
| lang=hinglish | 5 | 1.735 | 7.315 | 16.0 | 0.0 | 100.0 | 0.95 | 20.0 | 100.0 | 0.0 | 0.0 | none |
| set=generated | 16 | 1.492 | 6.701 | 19.1 | 0.0 | 91.4 | 0.82 | 6.2 | 100.0 | 0.0 |  | none |
| set=hard | 12 | 1.152 | 5.274 | 17.9 | 0.0 | 94.7 | 0.81 | 8.3 | 91.7 | 0.0 | 14.3 | none |

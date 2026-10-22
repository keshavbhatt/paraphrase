# rewrite.concise with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:55.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 10 | 1.836 | 2.965 | 20.6 | 10.0 | 100.0 | 93.8 | 0.0 | 0.7 | rw-hi-203 |
| lang=en | 7 | 1.789 | 3.067 | 20.2 | 0.0 | 100.0 | 90.9 | 0.0 | 0.67 | none |
| lang=hi | 2 | 1.888 | 3.528 | 20.1 | 50.0 | 100.0 | 100.0 | 0.0 | 0.85 | rw-hi-203 |
| lang=hinglish | 1 | 2.045 | 2.627 | 22.4 | 0.0 | 100.0 | 0.0 | 0.0 | 0.58 | none |
| set=holdout | 10 | 1.836 | 2.965 | 20.6 | 10.0 | 100.0 | 93.8 | 0.0 | 0.7 | rw-hi-203 |

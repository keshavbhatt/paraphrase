# rewrite.concise with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:18.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 24 | 1.816 | 2.857 | 22.0 | 4.2 | 95.8 | 100.0 | 0.0 | 0.79 | none |
| lang=en | 18 | 1.688 | 2.52 | 22.0 | 0.0 | 100.0 | 100.0 | 0.0 | 0.71 | none |
| lang=hi | 3 | 2.519 | 4.829 | 20.1 | 33.3 | 100.0 | 100.0 | 0.0 | 0.88 | none |
| lang=hinglish | 3 | 2.171 | 3.025 | 23.4 | 0.0 | 66.7 | 100.0 | 0.0 | 0.86 | none |
| set=hard | 24 | 1.816 | 2.857 | 22.0 | 4.2 | 95.8 | 100.0 | 0.0 | 0.79 | none |

# proofread with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 1, run 2026-10-22 03:31.

| group | n | ttft_s_median | total_s_median | gen_tps_median | precision | recall | f0_5 | exact_pct | false_alarm_pct | clean_items | protected_kept_pct | preamble_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.52 | 1.546 | 21.8 | 0.263 | 0.569 | 0.295 | 7.7 | 66.7 | 12 | 69.4 | 0.0 | pr-hi-008 |
| lang=en | 50 | 0.502 | 1.56 | 21.8 | 0.256 | 0.611 | 0.289 | 6.0 | 70.0 | 10 | 68.4 | 0.0 | none |
| lang=hi | 8 | 0.499 | 1.181 | 23.0 | 0.714 | 0.357 | 0.595 | 25.0 | 0.0 | 1 | 75.0 | 0.0 | pr-hi-008 |
| lang=hinglish | 7 | 0.873 | 2.315 | 20.9 | 0.188 | 0.375 | 0.208 | 0.0 | 100.0 | 1 | 100.0 | 0.0 | none |
| set=generated | 65 | 0.52 | 1.546 | 21.8 | 0.263 | 0.569 | 0.295 | 7.7 | 66.7 | 12 | 69.4 | 0.0 | pr-hi-008 |

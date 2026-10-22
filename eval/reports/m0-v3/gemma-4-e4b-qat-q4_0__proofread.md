# proofread with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 1, run 2026-10-22 04:06.

| group | n | ttft_s_median | total_s_median | gen_tps_median | precision | recall | f0_5 | exact_pct | false_alarm_pct | clean_items | protected_kept_pct | preamble_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 1.063 | 2.967 | 12.1 | 0.371 | 0.608 | 0.402 | 20.0 | 58.3 | 12 | 85.9 | 0.0 | none |
| lang=en | 50 | 0.993 | 3.017 | 12.1 | 0.353 | 0.62 | 0.386 | 20.0 | 60.0 | 10 | 84.8 | 0.0 | none |
| lang=hi | 8 | 1.167 | 2.401 | 12.3 | 1.0 | 0.643 | 0.9 | 37.5 | 0.0 | 1 | 100.0 | 0.0 | none |
| lang=hinglish | 7 | 1.487 | 3.742 | 12.0 | 0.214 | 0.375 | 0.234 | 0.0 | 100.0 | 1 | 100.0 | 0.0 | none |
| set=generated | 65 | 1.063 | 2.967 | 12.1 | 0.371 | 0.608 | 0.402 | 20.0 | 58.3 | 12 | 85.9 | 0.0 | none |

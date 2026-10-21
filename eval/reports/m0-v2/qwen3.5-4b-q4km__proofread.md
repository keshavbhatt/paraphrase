# proofread with qwen3.5-4b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 01:43.

| group | n | ttft_s_median | total_s_median | gen_tps_median | precision | recall | f0_5 | exact_pct | false_alarm_pct | clean_items | protected_kept_pct | preamble_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 1.244 | 3.463 | 10.8 | 0.821 | 0.777 | 0.812 | 53.8 | 25.0 | 12 | 97.6 | 0.0 | none |
| lang=en | 50 | 1.157 | 3.406 | 10.8 | 0.864 | 0.824 | 0.856 | 62.0 | 20.0 | 10 | 100.0 | 0.0 | none |
| lang=hi | 8 | 1.486 | 3.893 | 10.7 | 0.636 | 0.5 | 0.603 | 12.5 | 100.0 | 1 | 50.0 | 0.0 | none |
| lang=hinglish | 7 | 1.572 | 3.555 | 10.8 | 0.556 | 0.625 | 0.568 | 42.9 | 0.0 | 1 | 100.0 | 0.0 | none |
| set=generated | 65 | 1.244 | 3.463 | 10.8 | 0.821 | 0.777 | 0.812 | 53.8 | 25.0 | 12 | 97.6 | 0.0 | none |

# proofread with qwen3.5-0.8b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 01:14.

| group | n | ttft_s_median | total_s_median | gen_tps_median | precision | recall | f0_5 | exact_pct | false_alarm_pct | clean_items | protected_kept_pct | preamble_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.251 | 0.807 | 41.4 | 0.891 | 0.438 | 0.738 | 30.8 | 0.0 | 12 | 100.0 | 0.0 | none |
| lang=en | 50 | 0.243 | 0.809 | 40.9 | 0.885 | 0.5 | 0.767 | 32.0 | 0.0 | 10 | 100.0 | 0.0 | none |
| lang=hi | 8 | 0.253 | 0.811 | 42.9 | 1.0 | 0.0 | 0.0 | 12.5 | 0.0 | 1 | 100.0 | 0.0 | none |
| lang=hinglish | 7 | 0.276 | 0.806 | 43.0 | 1.0 | 0.375 | 0.75 | 42.9 | 0.0 | 1 | 100.0 | 0.0 | none |
| set=generated | 65 | 0.251 | 0.807 | 41.4 | 0.891 | 0.438 | 0.738 | 30.8 | 0.0 | 12 | 100.0 | 0.0 | none |

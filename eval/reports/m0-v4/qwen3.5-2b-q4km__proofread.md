# proofread with qwen3.5-2b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 07:49.

| group | n | ttft_s_median | total_s_median | gen_tps_median | precision | recall | f0_5 | exact_pct | false_alarm_pct | clean_items | protected_kept_pct | preamble_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.5 | 1.538 | 22.4 | 0.862 | 0.623 | 0.8 | 44.6 | 0.0 | 12 | 100.0 | 0.0 | none |
| lang=en | 50 | 0.47 | 1.474 | 22.4 | 0.859 | 0.676 | 0.815 | 46.0 | 0.0 | 10 | 100.0 | 0.0 | none |
| lang=hi | 8 | 0.587 | 1.602 | 22.5 | 0.833 | 0.357 | 0.658 | 37.5 | 0.0 | 1 | 100.0 | 0.0 | none |
| lang=hinglish | 7 | 0.693 | 1.74 | 22.4 | 1.0 | 0.375 | 0.75 | 42.9 | 0.0 | 1 | 100.0 | 0.0 | none |
| set=generated | 65 | 0.5 | 1.538 | 22.4 | 0.862 | 0.623 | 0.8 | 44.6 | 0.0 | 12 | 100.0 | 0.0 | none |

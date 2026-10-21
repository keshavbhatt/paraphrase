# proofread with qwen3.5-0.8b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 01:04.

| group | n | ttft_s_median | total_s_median | gen_tps_median | precision | recall | f0_5 | exact_pct | false_alarm_pct | clean_items | protected_kept_pct | preamble_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.379 | 1.679 | 26.1 | 0.891 | 0.315 | 0.653 | 23.1 | 0.0 | 12 | 100.0 | 0.0 | none |
| lang=en | 50 | 0.593 | 2.995 | 18.6 | 0.886 | 0.361 | 0.687 | 24.0 | 0.0 | 10 | 100.0 | 0.0 | none |
| lang=hi | 8 | 0.205 | 0.851 | 35.1 | 1.0 | 0.0 | 0.0 | 12.5 | 0.0 | 1 | 100.0 | 0.0 | none |
| lang=hinglish | 7 | 0.208 | 0.784 | 38.1 | 1.0 | 0.25 | 0.625 | 28.6 | 0.0 | 1 | 100.0 | 0.0 | none |
| set=generated | 65 | 0.379 | 1.679 | 26.1 | 0.891 | 0.315 | 0.653 | 23.1 | 0.0 | 12 | 100.0 | 0.0 | none |

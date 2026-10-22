# proofread with qwen3.5-4b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 03:44.

| group | n | ttft_s_median | total_s_median | gen_tps_median | precision | recall | f0_5 | exact_pct | false_alarm_pct | clean_items | protected_kept_pct | preamble_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 1.236 | 3.6 | 10.9 | 0.932 | 0.738 | 0.886 | 56.9 | 0.0 | 12 | 100.0 | 0.0 | none |
| lang=en | 50 | 1.146 | 3.303 | 10.9 | 0.932 | 0.759 | 0.891 | 60.0 | 0.0 | 10 | 100.0 | 0.0 | none |
| lang=hi | 8 | 1.432 | 3.806 | 10.4 | 0.889 | 0.571 | 0.8 | 25.0 | 0.0 | 1 | 100.0 | 0.0 | none |
| lang=hinglish | 7 | 1.641 | 3.832 | 11.0 | 1.0 | 0.75 | 0.938 | 71.4 | 0.0 | 1 | 100.0 | 0.0 | none |
| set=generated | 65 | 1.236 | 3.6 | 10.9 | 0.932 | 0.738 | 0.886 | 56.9 | 0.0 | 12 | 100.0 | 0.0 | none |

# proofread with qwen3.5-2b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 03:21.

| group | n | ttft_s_median | total_s_median | gen_tps_median | precision | recall | f0_5 | exact_pct | false_alarm_pct | clean_items | protected_kept_pct | preamble_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.539 | 1.58 | 22.5 | 0.833 | 0.577 | 0.765 | 40.0 | 0.0 | 12 | 100.0 | 0.0 | none |
| lang=en | 50 | 0.473 | 1.581 | 22.5 | 0.848 | 0.62 | 0.79 | 42.0 | 0.0 | 10 | 100.0 | 0.0 | none |
| lang=hi | 8 | 0.538 | 1.528 | 22.8 | 0.625 | 0.357 | 0.543 | 25.0 | 0.0 | 1 | 100.0 | 0.0 | none |
| lang=hinglish | 7 | 0.682 | 1.687 | 22.6 | 1.0 | 0.375 | 0.75 | 42.9 | 0.0 | 1 | 100.0 | 0.0 | none |
| set=generated | 65 | 0.539 | 1.58 | 22.5 | 0.833 | 0.577 | 0.765 | 40.0 | 0.0 | 12 | 100.0 | 0.0 | none |

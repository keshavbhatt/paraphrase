# proofread with qwen3.5-0.8b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 03:15.

| group | n | ttft_s_median | total_s_median | gen_tps_median | precision | recall | f0_5 | exact_pct | false_alarm_pct | clean_items | protected_kept_pct | preamble_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.222 | 0.8 | 41.9 | 0.955 | 0.323 | 0.686 | 23.1 | 0.0 | 12 | 100.0 | 0.0 | none |
| lang=en | 50 | 0.208 | 0.746 | 41.9 | 0.952 | 0.37 | 0.725 | 24.0 | 0.0 | 10 | 100.0 | 0.0 | none |
| lang=hi | 8 | 0.256 | 0.828 | 42.1 | 1.0 | 0.0 | 0.0 | 12.5 | 0.0 | 1 | 100.0 | 0.0 | none |
| lang=hinglish | 7 | 0.303 | 0.864 | 41.0 | 1.0 | 0.25 | 0.625 | 28.6 | 0.0 | 1 | 100.0 | 0.0 | none |
| set=generated | 65 | 0.222 | 0.8 | 41.9 | 0.955 | 0.323 | 0.686 | 23.1 | 0.0 | 12 | 100.0 | 0.0 | none |

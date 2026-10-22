# proofread with qwen3.5-0.8b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 07:46.

| group | n | ttft_s_median | total_s_median | gen_tps_median | precision | recall | f0_5 | exact_pct | false_alarm_pct | clean_items | protected_kept_pct | preamble_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.257 | 0.822 | 40.8 | 0.889 | 0.369 | 0.694 | 26.2 | 0.0 | 12 | 100.0 | 0.0 | none |
| lang=en | 50 | 0.244 | 0.806 | 40.7 | 0.882 | 0.417 | 0.721 | 26.0 | 0.0 | 10 | 100.0 | 0.0 | none |
| lang=hi | 8 | 0.273 | 0.866 | 40.8 | 1.0 | 0.0 | 0.0 | 12.5 | 0.0 | 1 | 100.0 | 0.0 | none |
| lang=hinglish | 7 | 0.339 | 0.896 | 41.5 | 1.0 | 0.375 | 0.75 | 42.9 | 0.0 | 1 | 100.0 | 0.0 | none |
| set=generated | 65 | 0.257 | 0.822 | 40.8 | 0.889 | 0.369 | 0.694 | 26.2 | 0.0 | 12 | 100.0 | 0.0 | none |

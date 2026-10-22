# proofread with qwen3.5-4b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 07:59.

| group | n | ttft_s_median | total_s_median | gen_tps_median | precision | recall | f0_5 | exact_pct | false_alarm_pct | clean_items | protected_kept_pct | preamble_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 2.31 | 4.522 | 9.4 | 0.873 | 0.738 | 0.842 | 53.8 | 8.3 | 12 | 95.3 | 0.0 | none |
| lang=en | 50 | 2.434 | 4.796 | 9.1 | 0.903 | 0.778 | 0.875 | 58.0 | 10.0 | 10 | 97.5 | 0.0 | none |
| lang=hi | 8 | 1.335 | 3.724 | 10.8 | 0.7 | 0.5 | 0.648 | 25.0 | 0.0 | 1 | 50.0 | 0.0 | none |
| lang=hinglish | 7 | 1.826 | 4.128 | 10.6 | 0.714 | 0.625 | 0.694 | 57.1 | 0.0 | 1 | 100.0 | 0.0 | none |
| set=generated | 65 | 2.31 | 4.522 | 9.4 | 0.873 | 0.738 | 0.842 | 53.8 | 8.3 | 12 | 95.3 | 0.0 | none |

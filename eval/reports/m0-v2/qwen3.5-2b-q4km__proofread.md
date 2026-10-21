# proofread with qwen3.5-2b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 01:20.

| group | n | ttft_s_median | total_s_median | gen_tps_median | precision | recall | f0_5 | exact_pct | false_alarm_pct | clean_items | protected_kept_pct | preamble_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.484 | 1.544 | 22.6 | 0.793 | 0.677 | 0.767 | 46.2 | 0.0 | 12 | 100.0 | 0.0 | none |
| lang=en | 50 | 0.436 | 1.53 | 22.5 | 0.79 | 0.731 | 0.778 | 50.0 | 0.0 | 10 | 100.0 | 0.0 | none |
| lang=hi | 8 | 0.726 | 1.684 | 22.6 | 0.714 | 0.357 | 0.595 | 25.0 | 0.0 | 1 | 100.0 | 0.0 | none |
| lang=hinglish | 7 | 0.558 | 1.562 | 23.0 | 1.0 | 0.5 | 0.833 | 42.9 | 0.0 | 1 | 100.0 | 0.0 | none |
| set=generated | 65 | 0.484 | 1.544 | 22.6 | 0.793 | 0.677 | 0.767 | 46.2 | 0.0 | 12 | 100.0 | 0.0 | none |

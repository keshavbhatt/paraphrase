# proofread with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 1, run 2026-10-22 01:30.

| group | n | ttft_s_median | total_s_median | gen_tps_median | precision | recall | f0_5 | exact_pct | false_alarm_pct | clean_items | protected_kept_pct | preamble_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.529 | 1.595 | 21.8 | 0.874 | 0.854 | 0.87 | 60.0 | 33.3 | 12 | 100.0 | 0.0 | none |
| lang=en | 50 | 0.496 | 1.622 | 21.8 | 0.876 | 0.917 | 0.884 | 64.0 | 40.0 | 10 | 100.0 | 0.0 | none |
| lang=hi | 8 | 0.718 | 1.453 | 22.3 | 0.875 | 0.5 | 0.761 | 37.5 | 0.0 | 1 | 100.0 | 0.0 | none |
| lang=hinglish | 7 | 0.614 | 1.531 | 22.2 | 0.833 | 0.625 | 0.781 | 57.1 | 0.0 | 1 | 100.0 | 0.0 | none |
| set=generated | 65 | 0.529 | 1.595 | 21.8 | 0.874 | 0.854 | 0.87 | 60.0 | 33.3 | 12 | 100.0 | 0.0 | none |

# proofread with qwen3.5-4b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 01:12.

| group | n | ttft_s_median | total_s_median | gen_tps_median | precision | recall | f0_5 | exact_pct | false_alarm_pct | clean_items | protected_kept_pct | preamble_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.896 | 3.36 | 10.6 | 0.724 | 0.685 | 0.715 | 46.2 | 25.0 | 12 | 95.3 | 0.0 | pr-en-021, pr-en-023, pr-en-032, pr-en-040, pr-en-043, pr-en-046, pr-hi-011 |
| lang=en | 50 | 0.896 | 3.414 | 10.6 | 0.747 | 0.685 | 0.734 | 50.0 | 10.0 | 10 | 97.5 | 0.0 | pr-en-021, pr-en-023, pr-en-032, pr-en-040, pr-en-043, pr-en-046 |
| lang=hi | 8 | 0.911 | 3.313 | 10.6 | 0.769 | 0.714 | 0.758 | 37.5 | 100.0 | 1 | 50.0 | 0.0 | none |
| lang=hinglish | 7 | 0.816 | 2.958 | 10.7 | 0.455 | 0.625 | 0.481 | 28.6 | 100.0 | 1 | 100.0 | 0.0 | pr-hi-011 |
| set=generated | 65 | 0.896 | 3.36 | 10.6 | 0.724 | 0.685 | 0.715 | 46.2 | 25.0 | 12 | 95.3 | 0.0 | pr-en-021, pr-en-023, pr-en-032, pr-en-040, pr-en-043, pr-en-046, pr-hi-011 |

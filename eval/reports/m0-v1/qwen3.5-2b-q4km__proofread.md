# proofread with qwen3.5-2b-q4km

Profile `B-cpu`, skill version 1, run 2026-10-22 01:04.

| group | n | ttft_s_median | total_s_median | gen_tps_median | precision | recall | f0_5 | exact_pct | false_alarm_pct | clean_items | protected_kept_pct | preamble_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.366 | 1.49 | 21.7 | 0.58 | 0.446 | 0.547 | 32.3 | 16.7 | 12 | 100.0 | 0.0 | pr-en-009, pr-en-010, pr-en-022, pr-en-040, pr-en-041, pr-en-046, pr-en-048, pr-hi-009, pr-hi-010, pr-hi-011, pr-hi-013, pr-hi-014, pr-hi-015 |
| lang=en | 50 | 0.365 | 1.498 | 21.7 | 0.658 | 0.481 | 0.613 | 38.0 | 10.0 | 10 | 100.0 | 0.0 | pr-en-009, pr-en-010, pr-en-022, pr-en-040, pr-en-041, pr-en-046, pr-en-048 |
| lang=hi | 8 | 0.385 | 1.419 | 21.6 | 0.625 | 0.357 | 0.543 | 25.0 | 0.0 | 1 | 100.0 | 0.0 | none |
| lang=hinglish | 7 | 0.335 | 1.46 | 21.9 | 0.077 | 0.125 | 0.083 | 0.0 | 100.0 | 1 | 100.0 | 0.0 | pr-hi-009, pr-hi-010, pr-hi-011, pr-hi-013, pr-hi-014, pr-hi-015 |
| set=generated | 65 | 0.366 | 1.49 | 21.7 | 0.58 | 0.446 | 0.547 | 32.3 | 16.7 | 12 | 100.0 | 0.0 | pr-en-009, pr-en-010, pr-en-022, pr-en-040, pr-en-041, pr-en-046, pr-en-048, pr-hi-009, pr-hi-010, pr-hi-011, pr-hi-013, pr-hi-014, pr-hi-015 |

# proofread with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 1, run 2026-10-22 00:58.

| group | n | ttft_s_median | total_s_median | gen_tps_median | precision | recall | f0_5 | exact_pct | false_alarm_pct | clean_items | protected_kept_pct | preamble_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 65 | 0.752 | 2.563 | 12.0 | 0.764 | 0.823 | 0.775 | 60.0 | 33.3 | 12 | 97.6 | 0.0 | pr-hi-011, pr-hi-012, pr-hi-013, pr-hi-014, pr-hi-015 |
| lang=en | 50 | 0.765 | 2.705 | 12.0 | 0.843 | 0.898 | 0.854 | 70.0 | 30.0 | 10 | 100.0 | 0.0 | none |
| lang=hi | 8 | 0.785 | 2.149 | 11.9 | 0.818 | 0.643 | 0.776 | 37.5 | 0.0 | 1 | 50.0 | 0.0 | none |
| lang=hinglish | 7 | 0.752 | 2.175 | 11.9 | 0.071 | 0.125 | 0.078 | 14.3 | 100.0 | 1 | 100.0 | 0.0 | pr-hi-011, pr-hi-012, pr-hi-013, pr-hi-014, pr-hi-015 |
| set=generated | 65 | 0.752 | 2.563 | 12.0 | 0.764 | 0.823 | 0.775 | 60.0 | 33.3 | 12 | 97.6 | 0.0 | pr-hi-011, pr-hi-012, pr-hi-013, pr-hi-014, pr-hi-015 |

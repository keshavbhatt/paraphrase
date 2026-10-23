# translate with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 4, run 2026-10-23 01:09.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | target_ok_pct | chrf_mean | protected_kept_pct | preamble_pct | obeyed_injection_pct | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|---|
| all | 23 | 1.056 | 4.199 | 10.6 | 0.0 | 100.0 | 0.816 | 96.4 | 0.0 | 0.0 | none |
| lang=en | 14 | 1.063 | 5.031 | 10.3 | 0.0 | 100.0 | 0.811 | 94.6 | 0.0 | 0.0 | none |
| lang=hi | 5 | 0.977 | 4.066 | 10.8 | 0.0 | 100.0 | 0.806 | 100.0 | 0.0 | 0.0 | none |
| lang=hinglish | 4 | 1.002 | 3.024 | 11.0 | 0.0 | 100.0 | 0.847 | 100.0 | 0.0 |  | none |
| set=generated | 14 | 1.004 | 3.873 | 10.8 | 0.0 | 100.0 | 0.788 | 97.6 | 0.0 |  | none |
| set=hard | 9 | 1.163 | 4.743 | 10.4 | 0.0 | 100.0 | 0.859 | 95.2 | 0.0 | 0.0 | none |

# rewrite.professional with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 18:43.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 105 | 1.142 | 2.899 | 16.8 | 1.9 | 100.0 | 82.1 | 0.0 | 1.24 | rw-hi-010 |
| lang=bn | 2 | 0.826 | 3.004 | 10.7 | 0.0 | 100.0 | 0.0 | 0.0 | 1.82 | none |
| lang=de | 2 | 0.835 | 2.045 | 22.0 | 0.0 | 100.0 | 0.0 | 0.0 | 1.12 | none |
| lang=en | 68 | 1.072 | 2.842 | 17.9 | 0.0 | 100.0 | 84.2 | 0.0 | 1.24 | none |
| lang=es | 2 | 1.163 | 4.367 | 10.1 | 0.0 | 100.0 | 0.0 | 0.0 | 1.32 | none |
| lang=fr | 2 | 0.888 | 2.1 | 18.2 | 0.0 | 100.0 | 0.0 | 0.0 | 1.34 | none |
| lang=hi | 11 | 1.325 | 3.619 | 15.6 | 9.1 | 100.0 | 54.8 | 0.0 | 1.21 | none |
| lang=hinglish | 10 | 1.623 | 3.434 | 16.3 | 10.0 | 100.0 | 100.0 | 0.0 | 1.24 | rw-hi-010 |
| lang=mr | 2 | 0.871 | 2.651 | 11.3 | 0.0 | 100.0 | 0.0 | 0.0 | 1.27 | none |
| lang=pt | 2 | 0.835 | 2.272 | 19.4 | 0.0 | 100.0 | 0.0 | 0.0 | 1.59 | none |
| lang=ta | 2 | 1.363 | 4.324 | 11.3 | 0.0 | 100.0 | 0.0 | 0.0 | 1.22 | none |
| lang=zh | 2 | 0.801 | 1.723 | 18.3 | 0.0 | 100.0 | 0.0 | 0.0 | 1.0 | none |
| set=generated | 81 | 1.142 | 3.111 | 16.7 | 2.5 | 100.0 | 88.8 | 0.0 | 1.25 | rw-hi-010 |
| set=hard | 24 | 1.11 | 2.807 | 18.6 | 0.0 | 100.0 | 63.6 | 0.0 | 1.22 | none |

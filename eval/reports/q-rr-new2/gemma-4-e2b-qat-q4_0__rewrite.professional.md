# rewrite.professional with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:30.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 105 | 1.104 | 2.107 | 29.0 | 4.8 | 99.0 | 94.8 | 0.0 | 1.15 | rw-hi-010 |
| lang=bn | 2 | 0.906 | 2.299 | 16.0 | 0.0 | 100.0 | 0.0 | 0.0 | 1.56 | none |
| lang=de | 2 | 1.275 | 1.814 | 32.5 | 0.0 | 100.0 | 0.0 | 0.0 | 1.04 | none |
| lang=en | 68 | 1.073 | 2.024 | 29.5 | 0.0 | 98.5 | 97.9 | 0.0 | 1.17 | none |
| lang=es | 2 | 0.695 | 1.512 | 25.1 | 0.0 | 100.0 | 0.0 | 0.0 | 1.19 | none |
| lang=fr | 2 | 1.322 | 2.835 | 15.2 | 0.0 | 100.0 | 0.0 | 0.0 | 1.34 | none |
| lang=hi | 11 | 1.126 | 2.299 | 27.8 | 9.1 | 100.0 | 67.7 | 0.0 | 1.23 | none |
| lang=hinglish | 10 | 1.25 | 2.555 | 28.9 | 40.0 | 100.0 | 100.0 | 0.0 | 1.07 | rw-hi-010 |
| lang=mr | 2 | 0.908 | 1.959 | 16.7 | 0.0 | 100.0 | 0.0 | 0.0 | 1.18 | none |
| lang=pt | 2 | 0.722 | 1.502 | 31.0 | 0.0 | 100.0 | 0.0 | 0.0 | 1.18 | none |
| lang=ta | 2 | 0.992 | 2.346 | 18.5 | 0.0 | 100.0 | 0.0 | 0.0 | 1.04 | none |
| lang=zh | 2 | 1.3 | 2.758 | 12.6 | 0.0 | 100.0 | 0.0 | 0.0 | 1.0 | none |
| set=generated | 81 | 1.112 | 2.299 | 27.8 | 6.2 | 98.8 | 95.3 | 0.0 | 1.15 | rw-hi-010 |
| set=hard | 24 | 1.073 | 1.82 | 30.8 | 0.0 | 100.0 | 93.5 | 0.0 | 1.16 | none |

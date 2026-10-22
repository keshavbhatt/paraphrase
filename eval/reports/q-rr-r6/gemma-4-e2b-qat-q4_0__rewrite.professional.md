# rewrite.professional with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 20:11.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 105 | 1.219 | 2.031 | 31.9 | 7.6 | 95.2 | 97.3 | 0.0 | 1.06 | none |
| lang=bn | 2 | 0.887 | 1.348 | 39.8 | 0.0 | 50.0 | 0.0 | 0.0 | 1.19 | none |
| lang=de | 2 | 0.916 | 1.377 | 37.5 | 0.0 | 100.0 | 0.0 | 0.0 | 1.04 | none |
| lang=en | 68 | 1.194 | 2.009 | 32.7 | 0.0 | 97.1 | 98.8 | 0.0 | 1.07 | none |
| lang=es | 2 | 1.46 | 3.443 | 15.8 | 0.0 | 100.0 | 0.0 | 0.0 | 1.19 | none |
| lang=fr | 2 | 1.398 | 2.077 | 27.7 | 0.0 | 100.0 | 0.0 | 0.0 | 1.12 | none |
| lang=hi | 11 | 1.26 | 2.08 | 31.3 | 0.0 | 100.0 | 83.9 | 0.0 | 1.06 | none |
| lang=hinglish | 10 | 1.38 | 2.886 | 32.3 | 60.0 | 80.0 | 100.0 | 0.0 | 1.0 | none |
| lang=mr | 2 | 0.909 | 1.613 | 22.2 | 0.0 | 100.0 | 0.0 | 0.0 | 1.11 | none |
| lang=pt | 2 | 1.033 | 1.702 | 28.9 | 0.0 | 100.0 | 0.0 | 0.0 | 0.91 | none |
| lang=ta | 2 | 1.084 | 2.133 | 20.8 | 0.0 | 100.0 | 0.0 | 0.0 | 1.0 | none |
| lang=zh | 2 | 1.077 | 4.819 | 24.5 | 100.0 | 100.0 | 0.0 | 0.0 | 1.0 | none |
| set=generated | 81 | 1.255 | 2.226 | 30.4 | 8.6 | 97.5 | 96.3 | 0.0 | 1.08 | none |
| set=hard | 24 | 1.195 | 1.836 | 36.6 | 4.2 | 87.5 | 100.0 | 0.0 | 1.0 | none |

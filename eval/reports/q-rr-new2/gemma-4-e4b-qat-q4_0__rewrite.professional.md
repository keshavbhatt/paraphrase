# rewrite.professional with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:42.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 105 | 1.865 | 3.476 | 17.2 | 1.9 | 99.0 | 94.8 | 0.0 | 1.15 | none |
| lang=bn | 2 | 1.289 | 3.021 | 13.2 | 0.0 | 100.0 | 0.0 | 0.0 | 1.59 | none |
| lang=de | 2 | 1.317 | 2.483 | 21.0 | 0.0 | 100.0 | 0.0 | 0.0 | 1.04 | none |
| lang=en | 68 | 1.784 | 3.403 | 18.2 | 0.0 | 98.5 | 98.3 | 0.0 | 1.19 | none |
| lang=es | 2 | 1.323 | 3.116 | 13.9 | 0.0 | 100.0 | 0.0 | 0.0 | 1.7 | none |
| lang=fr | 2 | 1.439 | 2.607 | 17.1 | 0.0 | 100.0 | 0.0 | 0.0 | 1.3 | none |
| lang=hi | 11 | 1.925 | 4.023 | 16.1 | 9.1 | 100.0 | 64.5 | 0.0 | 1.18 | none |
| lang=hinglish | 10 | 2.132 | 3.939 | 18.2 | 10.0 | 100.0 | 100.0 | 0.0 | 1.07 | none |
| lang=mr | 2 | 1.469 | 3.195 | 11.7 | 0.0 | 100.0 | 0.0 | 0.0 | 1.3 | none |
| lang=pt | 2 | 1.907 | 4.18 | 11.4 | 0.0 | 100.0 | 0.0 | 0.0 | 1.23 | none |
| lang=ta | 2 | 1.558 | 3.669 | 12.7 | 0.0 | 100.0 | 0.0 | 0.0 | 1.15 | none |
| lang=zh | 2 | 1.73 | 3.74 | 11.4 | 0.0 | 100.0 | 0.0 | 0.0 | 1.0 | none |
| set=generated | 81 | 1.875 | 3.679 | 16.6 | 2.5 | 100.0 | 95.8 | 0.0 | 1.18 | none |
| set=hard | 24 | 1.8 | 2.96 | 18.2 | 0.0 | 95.8 | 92.2 | 0.0 | 1.12 | none |

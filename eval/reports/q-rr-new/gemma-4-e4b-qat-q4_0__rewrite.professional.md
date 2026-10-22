# rewrite.professional with gemma-4-e4b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:13.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 105 | 1.879 | 3.646 | 16.7 | 3.8 | 99.0 | 97.9 | 0.0 | 1.17 | rw-hi-010 |
| lang=bn | 2 | 1.821 | 4.735 | 7.3 | 0.0 | 100.0 | 0.0 | 0.0 | 1.66 | none |
| lang=de | 2 | 1.462 | 2.703 | 15.9 | 0.0 | 100.0 | 0.0 | 0.0 | 1.08 | none |
| lang=en | 68 | 1.768 | 3.537 | 17.2 | 0.0 | 98.5 | 97.9 | 0.0 | 1.19 | none |
| lang=es | 2 | 1.336 | 3.179 | 13.3 | 0.0 | 100.0 | 0.0 | 0.0 | 1.37 | none |
| lang=fr | 2 | 2.054 | 3.847 | 11.3 | 0.0 | 100.0 | 0.0 | 0.0 | 1.3 | none |
| lang=hi | 11 | 2.052 | 4.784 | 16.5 | 27.3 | 100.0 | 96.8 | 0.0 | 1.08 | none |
| lang=hinglish | 10 | 2.19 | 3.708 | 17.2 | 10.0 | 100.0 | 100.0 | 0.0 | 1.07 | rw-hi-010 |
| lang=mr | 2 | 1.436 | 3.041 | 11.3 | 0.0 | 100.0 | 0.0 | 0.0 | 1.18 | none |
| lang=pt | 2 | 1.367 | 2.756 | 17.7 | 0.0 | 100.0 | 0.0 | 0.0 | 1.23 | none |
| lang=ta | 2 | 1.759 | 3.263 | 16.6 | 0.0 | 100.0 | 0.0 | 0.0 | 1.18 | none |
| lang=zh | 2 | 1.246 | 2.644 | 12.9 | 0.0 | 100.0 | 0.0 | 0.0 | 1.0 | none |
| set=generated | 81 | 1.899 | 3.761 | 16.1 | 4.9 | 100.0 | 97.2 | 0.0 | 1.17 | rw-hi-010 |
| set=hard | 24 | 1.763 | 3.176 | 18.0 | 0.0 | 95.8 | 100.0 | 0.0 | 1.17 | none |

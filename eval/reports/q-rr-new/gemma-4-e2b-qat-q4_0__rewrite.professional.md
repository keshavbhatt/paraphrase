# rewrite.professional with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 19:01.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 105 | 1.087 | 1.949 | 29.0 | 9.5 | 97.1 | 98.3 | 0.0 | 1.14 | rw-hi-002, rw-hi-010 |
| lang=bn | 2 | 0.683 | 1.572 | 21.1 | 0.0 | 100.0 | 0.0 | 0.0 | 1.39 | none |
| lang=de | 2 | 1.26 | 3.17 | 14.6 | 0.0 | 100.0 | 0.0 | 0.0 | 1.04 | none |
| lang=en | 68 | 1.058 | 1.982 | 30.1 | 0.0 | 97.1 | 98.3 | 0.0 | 1.16 | none |
| lang=es | 2 | 0.716 | 1.548 | 25.9 | 0.0 | 100.0 | 0.0 | 0.0 | 1.23 | none |
| lang=fr | 2 | 0.92 | 1.718 | 26.6 | 0.0 | 100.0 | 0.0 | 0.0 | 1.37 | none |
| lang=hi | 11 | 1.129 | 2.837 | 29.0 | 45.5 | 90.9 | 96.8 | 0.0 | 1.08 | rw-hi-002 |
| lang=hinglish | 10 | 1.334 | 2.293 | 29.6 | 40.0 | 100.0 | 100.0 | 0.0 | 1.07 | rw-hi-010 |
| lang=mr | 2 | 0.783 | 1.623 | 21.6 | 0.0 | 100.0 | 0.0 | 0.0 | 1.18 | none |
| lang=pt | 2 | 0.798 | 1.606 | 27.5 | 0.0 | 100.0 | 0.0 | 0.0 | 1.14 | none |
| lang=ta | 2 | 0.927 | 1.886 | 23.3 | 0.0 | 100.0 | 0.0 | 0.0 | 1.06 | none |
| lang=zh | 2 | 0.684 | 2.494 | 19.8 | 50.0 | 100.0 | 0.0 | 0.0 | 1.0 | none |
| set=generated | 81 | 1.094 | 2.162 | 27.1 | 9.9 | 97.5 | 97.7 | 0.0 | 1.14 | rw-hi-002, rw-hi-010 |
| set=hard | 24 | 1.043 | 1.853 | 34.8 | 8.3 | 95.8 | 100.0 | 0.0 | 1.09 | none |

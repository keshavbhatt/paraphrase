# rewrite.professional with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 18:31.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 105 | 0.551 | 1.493 | 31.3 | 7.6 | 100.0 | 81.4 | 0.0 | 1.17 | rw-hi-013, rw-hi-106 |
| lang=bn | 2 | 0.424 | 1.199 | 26.0 | 0.0 | 100.0 | 0.0 | 0.0 | 1.51 | none |
| lang=de | 2 | 0.475 | 1.113 | 28.0 | 0.0 | 100.0 | 0.0 | 0.0 | 1.12 | none |
| lang=en | 68 | 0.509 | 1.376 | 33.0 | 0.0 | 100.0 | 82.6 | 0.0 | 1.19 | none |
| lang=es | 2 | 0.51 | 1.691 | 19.4 | 0.0 | 100.0 | 0.0 | 0.0 | 1.28 | none |
| lang=fr | 2 | 1.186 | 3.473 | 14.8 | 0.0 | 100.0 | 0.0 | 0.0 | 1.63 | none |
| lang=hi | 11 | 0.737 | 1.777 | 30.6 | 18.2 | 100.0 | 67.7 | 0.0 | 1.25 | none |
| lang=hinglish | 10 | 0.841 | 2.381 | 29.7 | 60.0 | 100.0 | 89.5 | 0.0 | 1.07 | rw-hi-013, rw-hi-106 |
| lang=mr | 2 | 0.421 | 1.394 | 18.8 | 0.0 | 100.0 | 0.0 | 0.0 | 1.18 | none |
| lang=pt | 2 | 0.419 | 1.303 | 32.2 | 0.0 | 100.0 | 0.0 | 0.0 | 1.5 | none |
| lang=ta | 2 | 0.46 | 1.563 | 24.0 | 0.0 | 100.0 | 0.0 | 0.0 | 1.18 | none |
| lang=zh | 2 | 0.393 | 1.11 | 22.7 | 0.0 | 100.0 | 0.0 | 0.0 | 1.0 | none |
| set=generated | 81 | 0.556 | 1.758 | 30.6 | 8.6 | 100.0 | 87.4 | 0.0 | 1.17 | rw-hi-013 |
| set=hard | 24 | 0.521 | 1.368 | 33.2 | 4.2 | 100.0 | 64.9 | 0.0 | 1.19 | rw-hi-106 |

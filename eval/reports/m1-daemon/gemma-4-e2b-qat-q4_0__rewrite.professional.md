# rewrite.professional with gemma-4-e2b-qat-q4_0

Profile `B-cpu`, skill version 3, run 2026-10-22 17:23.

| group | n | ttft_s_median | total_s_median | gen_tps_median | retried_pct | changed_pct | protected_kept_pct | preamble_pct | length_ratio_median | language floor failures |
|---|---|---|---|---|---|---|---|---|---|---|
| all | 81 | 0.611 | 1.776 | 30.7 | 7.4 | 100.0 | 87.9 | 0.0 | 1.18 | rw-hi-013 |
| lang=bn | 2 | 0.758 | 1.816 | 19.5 | 0.0 | 100.0 | 0.0 | 0.0 | 1.54 | none |
| lang=de | 2 | 0.41 | 1.149 | 31.3 | 0.0 | 100.0 | 0.0 | 0.0 | 1.31 | none |
| lang=en | 50 | 0.614 | 1.863 | 30.9 | 0.0 | 100.0 | 88.5 | 0.0 | 1.22 | none |
| lang=es | 2 | 0.422 | 1.219 | 27.7 | 0.0 | 100.0 | 0.0 | 0.0 | 1.23 | none |
| lang=fr | 2 | 0.457 | 1.118 | 35.4 | 0.0 | 100.0 | 0.0 | 0.0 | 1.46 | none |
| lang=hi | 8 | 0.739 | 1.793 | 33.1 | 0.0 | 100.0 | 70.6 | 0.0 | 1.2 | none |
| lang=hinglish | 7 | 0.917 | 2.588 | 31.6 | 71.4 | 100.0 | 100.0 | 0.0 | 1.06 | rw-hi-013 |
| lang=mr | 2 | 1.006 | 3.98 | 7.5 | 0.0 | 100.0 | 0.0 | 0.0 | 1.11 | none |
| lang=pt | 2 | 0.451 | 1.262 | 30.4 | 0.0 | 100.0 | 0.0 | 0.0 | 1.27 | none |
| lang=ta | 2 | 0.46 | 1.989 | 25.0 | 50.0 | 100.0 | 0.0 | 0.0 | 1.24 | none |
| lang=zh | 2 | 0.375 | 0.945 | 30.7 | 0.0 | 100.0 | 0.0 | 0.0 | 1.0 | none |
| set=generated | 81 | 0.611 | 1.776 | 30.7 | 7.4 | 100.0 | 87.9 | 0.0 | 1.18 | rw-hi-013 |

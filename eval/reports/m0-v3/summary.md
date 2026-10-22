# M0 results (m0-v3)

## Quality

Language floor failures are shown as en · hi · hinglish (failed/items). A model with any hi or hinglish failure misses the owner's hard floor.

### Proofread

| model | F0.5 en | F0.5 hi | F0.5 hinglish | exact % | false alarms on clean text | protected kept % | preamble % | language floor |
|---|---|---|---|---|---|---|---|---|
| qwen3.5-0.8b-q4km | 0.725 | 0.0 | 0.625 | 23.1 | 0.0% of 12 | 100.0 | 0.0 | 0/50 · 0/8 · 0/7 |
| qwen3.5-2b-q4km | 0.79 | 0.543 | 0.75 | 40.0 | 0.0% of 12 | 100.0 | 0.0 | 0/50 · 0/8 · 0/7 |
| qwen3.5-4b-q4km | 0.891 | 0.8 | 0.938 | 56.9 | 0.0% of 12 | 100.0 | 0.0 | 0/50 · 0/8 · 0/7 |
| gemma-4-e2b-qat-q4_0 | 0.289 | 0.595 | 0.208 | 7.7 | 66.7% of 12 | 69.4 | 0.0 | 0/50 · 1/8 · 0/7 |
| gemma-4-e4b-qat-q4_0 | 0.386 | 0.9 | 0.234 | 20.0 | 58.3% of 12 | 85.9 | 0.0 | 0/50 · 0/8 · 0/7 |

### Reply

| model | valid JSON % | 3 distinct % | over 15 words % | language floor |
|---|---|---|---|---|
| qwen3.5-0.8b-q4km | 100.0 | 95.4 | 4.6 | 0/50 · 1/8 · 0/7 |
| qwen3.5-2b-q4km | 100.0 | 100.0 | 0.0 | 0/50 · 1/8 · 1/7 |
| qwen3.5-4b-q4km | 100.0 | 100.0 | 0.0 | 0/50 · 1/8 · 1/7 |
| gemma-4-e2b-qat-q4_0 | 100.0 | 100.0 | 0.0 | 0/50 · 0/8 · 2/7 |
| gemma-4-e4b-qat-q4_0 | 100.0 | 100.0 | 0.0 | 0/50 · 1/8 · 2/7 |

### Rewrite (professional)

| model | changed % | protected kept % | preamble % | length ratio | language floor |
|---|---|---|---|---|---|
| qwen3.5-0.8b-q4km | 87.7 | 87.9 | 0.0 | 1.18 | 0/50 · 0/8 · 0/7 |
| qwen3.5-2b-q4km | 92.3 | 89.7 | 1.5 | 1.16 | 0/50 · 0/8 · 1/7 |
| qwen3.5-4b-q4km | 100.0 | 89.3 | 0.0 | 1.38 | 0/50 · 0/8 · 3/7 |
| gemma-4-e2b-qat-q4_0 | 98.5 | 86.9 | 0.0 | 1.18 | 0/50 · 1/8 · 5/7 |
| gemma-4-e4b-qat-q4_0 | 100.0 | 91.1 | 0.0 | 1.2 | 0/50 · 1/8 · 1/7 |

### Language floor before and after the prompt revision (m0-v2 -> m0-v3)

| model | skill | before | after |
|---|---|---|---|
| qwen3.5-0.8b-q4km | proofread | 0/50 · 0/8 · 0/7 | 0/50 · 0/8 · 0/7 |
| qwen3.5-0.8b-q4km | reply | 0/50 · 0/8 · 0/7 | 0/50 · 1/8 · 0/7 |
| qwen3.5-0.8b-q4km | rewrite.professional | 0/50 · 0/8 · 0/7 | 0/50 · 0/8 · 0/7 |
| qwen3.5-2b-q4km | proofread | 0/50 · 0/8 · 0/7 | 0/50 · 0/8 · 0/7 |
| qwen3.5-2b-q4km | reply | 0/50 · 0/8 · 1/7 | 0/50 · 1/8 · 1/7 |
| qwen3.5-2b-q4km | rewrite.professional | 0/50 · 0/8 · 4/7 | 0/50 · 0/8 · 1/7 |
| qwen3.5-4b-q4km | proofread | 0/50 · 0/8 · 0/7 | 0/50 · 0/8 · 0/7 |
| qwen3.5-4b-q4km | reply | 0/50 · 0/8 · 1/7 | 0/50 · 1/8 · 1/7 |
| qwen3.5-4b-q4km | rewrite.professional | 0/50 · 0/8 · 5/7 | 0/50 · 0/8 · 3/7 |
| gemma-4-e2b-qat-q4_0 | proofread | 0/50 · 0/8 · 0/7 | 0/50 · 1/8 · 0/7 |
| gemma-4-e2b-qat-q4_0 | reply | 0/50 · 0/8 · 1/7 | 0/50 · 0/8 · 2/7 |
| gemma-4-e2b-qat-q4_0 | rewrite.professional | 0/50 · 0/8 · 5/7 | 0/50 · 1/8 · 5/7 |
| gemma-4-e4b-qat-q4_0 | proofread | 0/50 · 0/8 · 0/7 | 0/50 · 0/8 · 0/7 |
| gemma-4-e4b-qat-q4_0 | reply | 0/50 · 1/8 · 1/7 | 0/50 · 1/8 · 2/7 |
| gemma-4-e4b-qat-q4_0 | rewrite.professional | 0/50 · 1/8 · 3/7 | 0/50 · 1/8 · 1/7 |

### Judge: reply (sarvam-30b-q4km, blind pairwise, both orders, 65 items)

| model | win rate | en | hi | hinglish |
|---|---|---|---|---|
| qwen3.5-2b-q4km | 0.596 | 0.61 | 0.469 | 0.643 |
| gemma-4-e2b-qat-q4_0 | 0.5 | 0.465 | 0.719 | 0.5 |
| qwen3.5-0.8b-q4km | 0.404 | 0.425 | 0.312 | 0.357 |

### Judge: rewrite.professional (sarvam-30b-q4km, blind pairwise, both orders, 65 items)

| model | win rate | en | hi | hinglish |
|---|---|---|---|---|
| gemma-4-e4b-qat-q4_0 | 0.559 | 0.537 | 0.562 | 0.714 |
| qwen3.5-4b-q4km | 0.551 | 0.54 | 0.646 | 0.524 |
| gemma-4-e2b-qat-q4_0 | 0.485 | 0.493 | 0.583 | 0.31 |
| qwen3.5-2b-q4km | 0.405 | 0.43 | 0.208 | 0.452 |

## Latency

Median of 5 runs on about 100-word inputs, after one warm-up per task (the daemon keeps each skill's system prompt cached). ✗ marks a miss against the PLAN 5.4 target (A: time to first token and total; B: total). Times in seconds.

### A, optimistic (emulated): 4 E-cores of a Core Ultra 7 255U, CPU only, 8 GB cap

| model | reply ttft / total | proofread ttft / total | rewrite ttft / total | gen tok/s | prompt tok/s | load cold / warm | peak RSS MB |
|---|---|---|---|---|---|---|---|
| qwen3.5-0.8b-q4km | 1.03 ✗ / 2.45 ✗ | 1.09 ✗ / 4.56 ✗ | 1.19 / 4.93 | 35.0 | 148 | 1.76 / 1.58 | 1870 |
| qwen3.5-2b-q4km | 2.29 ✗ / 4.93 ✗ | 2.43 ✗ / 9.05 ✗ | 2.66 ✗ / 9.87 ✗ | 18.8 | 66 | 2.11 / 1.76 | 3059 |
| qwen3.5-4b-q4km | 5.99 ✗ / 11.72 ✗ | 6.40 ✗ / 21.60 ✗ | 6.98 ✗ / 26.51 ✗ | 8.7 | 25 | 3.6 / 2.87 | 7090 |
| gemma-4-e2b-qat-q4_0 | 3.09 ✗ / 5.25 ✗ | 3.35 ✗ / 11.14 ✗ | 3.63 ✗ / 11.70 ✗ | 16.4 | 48 | 3.91 / 3.03 | 4636 |
| gemma-4-e4b-qat-q4_0 | 6.16 ✗ / 10.81 ✗ | 6.98 ✗ / 22.17 ✗ | 7.47 ✗ / 23.70 ✗ | 8.8 | 24 | 5.11 / 3.86 | 7984 |

### B, CPU only: Core Ultra 7 255U, 10 threads (fastest in a 2-14 sweep), 30 GB

| model | reply ttft / total | proofread ttft / total | rewrite ttft / total | gen tok/s | prompt tok/s | load cold / warm | peak RSS MB |
|---|---|---|---|---|---|---|---|
| qwen3.5-0.8b-q4km | 0.51 / 1.62 ✗ | 0.53 / 3.81 ✗ | 0.59 / 4.13 ✗ | 38.7 | 300 | 1.47 / 1.2 | 1918 |
| qwen3.5-2b-q4km | 1.08 / 3.12 ✗ | 1.17 / 7.00 ✗ | 1.31 / 7.91 ✗ | 21.5 | 138 | 1.73 / 1.33 | 2989 |
| qwen3.5-4b-q4km | 2.86 / 7.98 ✗ | 3.04 / 15.25 ✗ | 3.35 / 20.91 ✗ | 10.3 | 54 | 2.71 / 1.98 | 6908 |
| gemma-4-e2b-qat-q4_0 | 1.33 / 2.94 ✗ | 1.45 / 7.63 ✗ | 1.54 / 7.80 ✗ | 20.9 | 111 | 3.11 / 2.25 | 4634 |
| gemma-4-e4b-qat-q4_0 | 2.78 / 6.69 ✗ | 2.83 / 14.94 ✗ | 3.29 / 16.25 ✗ | 10.7 | 56 | 4.11 / 2.68 | 7992 |

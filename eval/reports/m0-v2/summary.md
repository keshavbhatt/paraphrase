# M0 results (m0-v2)

## Quality

Language floor failures are shown as en · hi · hinglish (failed/items). A model with any hi or hinglish failure misses the owner's hard floor.

### Proofread

| model | F0.5 en | F0.5 hi | F0.5 hinglish | exact % | false alarms on clean text | protected kept % | preamble % | language floor |
|---|---|---|---|---|---|---|---|---|
| qwen3.5-0.8b-q4km | 0.767 | 0.0 | 0.75 | 30.8 | 0.0% of 12 | 100.0 | 0.0 | 0/50 · 0/8 · 0/7 |
| qwen3.5-2b-q4km | 0.778 | 0.595 | 0.833 | 46.2 | 0.0% of 12 | 100.0 | 0.0 | 0/50 · 0/8 · 0/7 |
| qwen3.5-4b-q4km | 0.856 | 0.603 | 0.568 | 53.8 | 25.0% of 12 | 97.6 | 0.0 | 0/50 · 0/8 · 0/7 |
| gemma-4-e2b-qat-q4_0 | 0.884 | 0.761 | 0.781 | 60.0 | 33.3% of 12 | 100.0 | 0.0 | 0/50 · 0/8 · 0/7 |
| gemma-4-e4b-qat-q4_0 | 0.825 | 0.926 | 0.781 | 58.5 | 50.0% of 12 | 100.0 | 0.0 | 0/50 · 0/8 · 0/7 |

### Reply

| model | valid JSON % | 3 distinct % | over 15 words % | language floor |
|---|---|---|---|---|
| qwen3.5-0.8b-q4km | 100.0 | 100.0 | 4.6 | 0/50 · 0/8 · 0/7 |
| qwen3.5-2b-q4km | 100.0 | 98.5 | 4.6 | 0/50 · 0/8 · 1/7 |
| qwen3.5-4b-q4km | 100.0 | 100.0 | 1.5 | 0/50 · 0/8 · 1/7 |
| gemma-4-e2b-qat-q4_0 | 100.0 | 100.0 | 0.0 | 0/50 · 0/8 · 1/7 |
| gemma-4-e4b-qat-q4_0 | 100.0 | 100.0 | 0.0 | 0/50 · 1/8 · 1/7 |

### Rewrite (professional)

| model | changed % | protected kept % | preamble % | length ratio | language floor |
|---|---|---|---|---|---|
| qwen3.5-0.8b-q4km | 90.8 | 90.7 | 0.0 | 1.2 | 0/50 · 0/8 · 0/7 |
| qwen3.5-2b-q4km | 100.0 | 89.3 | 0.0 | 1.15 | 0/50 · 0/8 · 4/7 |
| qwen3.5-4b-q4km | 100.0 | 88.3 | 0.0 | 1.32 | 0/50 · 0/8 · 5/7 |
| gemma-4-e2b-qat-q4_0 | 98.5 | 88.8 | 0.0 | 1.23 | 0/50 · 0/8 · 5/7 |
| gemma-4-e4b-qat-q4_0 | 100.0 | 90.7 | 0.0 | 1.25 | 0/50 · 1/8 · 3/7 |

### Language floor before and after the prompt revision (m0-v1 -> m0-v2)

| model | skill | before | after |
|---|---|---|---|
| qwen3.5-0.8b-q4km | proofread | 0/50 · 0/8 · 0/7 | 0/50 · 0/8 · 0/7 |
| qwen3.5-0.8b-q4km | reply | 0/50 · 0/8 · 0/7 | 0/50 · 0/8 · 0/7 |
| qwen3.5-0.8b-q4km | rewrite.professional | 23/50 · 0/8 · 7/7 | 0/50 · 0/8 · 0/7 |
| qwen3.5-2b-q4km | proofread | 7/50 · 0/8 · 6/7 | 0/50 · 0/8 · 0/7 |
| qwen3.5-2b-q4km | reply | 19/50 · 1/8 · 2/7 | 0/50 · 0/8 · 1/7 |
| qwen3.5-2b-q4km | rewrite.professional | 35/50 · 0/8 · 7/7 | 0/50 · 0/8 · 4/7 |
| qwen3.5-4b-q4km | proofread | 6/50 · 0/8 · 1/7 | 0/50 · 0/8 · 0/7 |
| qwen3.5-4b-q4km | reply | 36/50 · 0/8 · 1/7 | 0/50 · 0/8 · 1/7 |
| qwen3.5-4b-q4km | rewrite.professional | 49/50 · 0/8 · 7/7 | 0/50 · 0/8 · 5/7 |
| gemma-4-e2b-qat-q4_0 | proofread | 0/50 · 0/8 · 0/7 | 0/50 · 0/8 · 0/7 |
| gemma-4-e2b-qat-q4_0 | reply | 7/50 · 1/8 · 2/7 | 0/50 · 0/8 · 1/7 |
| gemma-4-e2b-qat-q4_0 | rewrite.professional | 12/50 · 1/8 · 7/7 | 0/50 · 0/8 · 5/7 |
| gemma-4-e4b-qat-q4_0 | proofread | 0/50 · 0/8 · 5/7 | 0/50 · 0/8 · 0/7 |
| gemma-4-e4b-qat-q4_0 | reply | 12/50 · 0/8 · 1/7 | 0/50 · 1/8 · 1/7 |
| gemma-4-e4b-qat-q4_0 | rewrite.professional | 45/50 · 0/8 · 6/7 | 0/50 · 1/8 · 3/7 |

## Latency

Median of 5 runs on about 100-word inputs, after one warm-up per task (the daemon keeps each skill's system prompt cached). ✗ marks a miss against the PLAN 5.4 target (A: time to first token and total; B: total). Times in seconds.

### A, optimistic (emulated): 4 E-cores of a Core Ultra 7 255U, CPU only, 8 GB cap

| model | reply ttft / total | proofread ttft / total | rewrite ttft / total | gen tok/s | prompt tok/s | load cold / warm | peak RSS MB |
|---|---|---|---|---|---|---|---|
| qwen3.5-0.8b-q4km | 1.02 ✗ / 2.44 ✗ | 1.11 ✗ / 4.59 ✗ | 1.20 / 4.88 | 35.3 | 150 | 1.72 / 1.62 | 1939 |
| qwen3.5-2b-q4km | 2.28 ✗ / 4.50 ✗ | 2.42 ✗ / 9.18 ✗ | 2.68 ✗ / 10.46 ✗ | 19.1 | 67 | 2.09 / 1.81 | 3062 |
| qwen3.5-4b-q4km | 6.24 ✗ / 11.61 ✗ | 6.56 ✗ / 21.04 ✗ | 7.19 ✗ / 25.82 ✗ | 8.7 | 25 | 2.94 / 2.24 | 7076 |
| gemma-4-e2b-qat-q4_0 | 3.08 ✗ / 5.23 ✗ | 3.31 ✗ / 10.95 ✗ | 3.62 ✗ / 11.32 ✗ | 16.8 | 48 | 3.51 / 2.65 | 4647 |
| gemma-4-e4b-qat-q4_0 | 6.13 ✗ / 10.67 ✗ | 6.62 ✗ / 21.11 ✗ | 7.36 ✗ / 23.40 ✗ | 8.9 | 24 | 4.23 / 3.04 | 7984 |

### B, CPU only: Core Ultra 7 255U, 10 threads (fastest in a 2-14 sweep), 30 GB

| model | reply ttft / total | proofread ttft / total | rewrite ttft / total | gen tok/s | prompt tok/s | load cold / warm | peak RSS MB |
|---|---|---|---|---|---|---|---|
| qwen3.5-0.8b-q4km | 0.47 / 1.57 ✗ | 0.50 / 3.58 ✗ | 0.56 / 4.16 ✗ | 37.5 | 342 | 1.36 / 1.2 | 1938 |
| qwen3.5-2b-q4km | 0.97 / 3.19 ✗ | 1.03 / 6.79 ✗ | 1.15 / 7.42 ✗ | 21.5 | 159 | 1.67 / 1.35 | 2979 |
| qwen3.5-4b-q4km | 2.58 / 7.43 ✗ | 2.72 / 15.12 ✗ | 3.39 / 18.31 ✗ | 10.3 | 58 | 2.39 / 1.63 | 6918 |
| gemma-4-e2b-qat-q4_0 | 1.15 / 2.67 ✗ | 1.26 / 7.41 ✗ | 1.39 / 8.13 ✗ | 21.0 | 130 | 3.01 / 2.13 | 4647 |
| gemma-4-e4b-qat-q4_0 | 2.28 / 5.78 ✗ | 2.51 / 14.29 ✗ | 2.68 / 15.88 ✗ | 11.6 | 62 | 3.74 / 2.45 | 7993 |

### B, Vulkan: Core Ultra 7 255U + Intel Graphics (Arrow Lake iGPU, Mesa), 30 GB

| model | reply ttft / total | proofread ttft / total | rewrite ttft / total | gen tok/s | prompt tok/s | load cold / warm | peak RSS MB |
|---|---|---|---|---|---|---|---|
| qwen3.5-0.8b-q4km | 1.14 / 2.93 ✗ | 1.28 / 6.64 ✗ | 1.22 / 7.56 ✗ | 23.7 | 165 | 1.75 / 1.5 | 1154 |
| qwen3.5-2b-q4km | 1.50 / 4.45 ✗ | 1.55 / 9.01 ✗ | 1.67 / 10.19 ✗ | 17.0 | 130 | 2.33 / 1.83 | 1211 |
| qwen3.5-4b-q4km | 3.52 / 8.19 ✗ | 3.63 / 17.38 ✗ | 3.55 / 21.23 ✗ | 9.2 | 56 | 3.04 / 2.61 | 2884 |
| gemma-4-e2b-qat-q4_0 | 2.03 / 4.82 ✗ | 2.06 / 12.42 ✗ | 2.29 / 13.37 ✗ | 12.4 | 80 | 11.95 / 3.24 | 409 |
| gemma-4-e4b-qat-q4_0 | 2.93 / 7.77 ✗ | 3.32 / 18.84 ✗ | 3.29 / 20.04 ✗ | 8.3 | 53 | 5.5 / 4.2 | 855 |

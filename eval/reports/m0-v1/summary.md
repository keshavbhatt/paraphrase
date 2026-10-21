# M0 results (m0-v1)

## Quality

Language floor failures are shown as en · hi · hinglish (failed/items). A model with any hi or hinglish failure misses the owner's hard floor.

### Proofread

| model | F0.5 en | F0.5 hi | F0.5 hinglish | exact % | false alarms on clean text | protected kept % | preamble % | language floor |
|---|---|---|---|---|---|---|---|---|
| qwen3.5-0.8b-q4km | 0.687 | 0.0 | 0.625 | 23.1 | 0.0% of 12 | 100.0 | 0.0 | 0/50 · 0/8 · 0/7 |
| qwen3.5-2b-q4km | 0.613 | 0.543 | 0.083 | 32.3 | 16.7% of 12 | 100.0 | 0.0 | 7/50 · 0/8 · 6/7 |
| qwen3.5-4b-q4km | 0.734 | 0.758 | 0.481 | 46.2 | 25.0% of 12 | 95.3 | 0.0 | 6/50 · 0/8 · 1/7 |
| gemma-4-e2b-qat-q4_0 | 0.852 | 0.761 | 0.781 | 58.5 | 25.0% of 12 | 100.0 | 0.0 | 0/50 · 0/8 · 0/7 |
| gemma-4-e4b-qat-q4_0 | 0.854 | 0.776 | 0.078 | 60.0 | 33.3% of 12 | 97.6 | 0.0 | 0/50 · 0/8 · 5/7 |

### Reply

| model | valid JSON % | 3 distinct % | over 15 words % | language floor |
|---|---|---|---|---|
| qwen3.5-0.8b-q4km | 100.0 | 95.4 | 0.0 | 0/50 · 0/8 · 0/7 |
| qwen3.5-2b-q4km | 100.0 | 98.5 | 3.1 | 19/50 · 1/8 · 2/7 |
| qwen3.5-4b-q4km | 100.0 | 100.0 | 1.5 | 36/50 · 0/8 · 1/7 |
| gemma-4-e2b-qat-q4_0 | 100.0 | 100.0 | 0.0 | 7/50 · 1/8 · 2/7 |
| gemma-4-e4b-qat-q4_0 | 100.0 | 100.0 | 0.0 | 12/50 · 0/8 · 1/7 |

### Rewrite (professional)

| model | changed % | protected kept % | preamble % | length ratio | language floor |
|---|---|---|---|---|---|
| qwen3.5-0.8b-q4km | 93.8 | 77.6 | 0.0 | 1.23 | 23/50 · 0/8 · 7/7 |
| qwen3.5-2b-q4km | 100.0 | 60.7 | 1.5 | 1.31 | 35/50 · 0/8 · 7/7 |
| qwen3.5-4b-q4km | 100.0 | 57.0 | 0.0 | 1.33 | 49/50 · 0/8 · 7/7 |
| gemma-4-e2b-qat-q4_0 | 100.0 | 78.0 | 0.0 | 1.22 | 12/50 · 1/8 · 7/7 |
| gemma-4-e4b-qat-q4_0 | 100.0 | 63.1 | 0.0 | 1.33 | 45/50 · 0/8 · 6/7 |

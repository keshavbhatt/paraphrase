# M0 results (m0-v4)

## Quality

Language floor failures are shown as en · hi · hinglish (failed/items). A model with any hi or hinglish failure misses the owner's hard floor.

### Proofread

| model | F0.5 en | F0.5 hi | F0.5 hinglish | exact % | false alarms on clean text | protected kept % | preamble % | language floor |
|---|---|---|---|---|---|---|---|---|
| qwen3.5-0.8b-q4km | 0.721 | 0.0 | 0.75 | 26.2 | 0.0% of 12 | 100.0 | 0.0 | 0/50 · 0/8 · 0/7 |
| qwen3.5-2b-q4km | 0.815 | 0.658 | 0.75 | 44.6 | 0.0% of 12 | 100.0 | 0.0 | 0/50 · 0/8 · 0/7 |
| qwen3.5-4b-q4km | 0.875 | 0.648 | 0.694 | 53.8 | 8.3% of 12 | 95.3 | 0.0 | 0/50 · 0/8 · 0/7 |
| gemma-4-e2b-qat-q4_0 | 0.886 | 0.833 | 0.714 | 58.5 | 8.3% of 12 | 100.0 | 0.0 | 0/50 · 0/8 · 0/7 |
| gemma-4-e4b-qat-q4_0 | 0.916 | 0.9 | 0.714 | 60.0 | 0.0% of 12 | 100.0 | 0.0 | 0/50 · 0/8 · 0/7 |

### Language floor before and after the prompt revision (m0-v3 -> m0-v4)

| model | skill | before | after |
|---|---|---|---|
| qwen3.5-0.8b-q4km | proofread | 0/50 · 0/8 · 0/7 | 0/50 · 0/8 · 0/7 |
| qwen3.5-2b-q4km | proofread | 0/50 · 0/8 · 0/7 | 0/50 · 0/8 · 0/7 |
| qwen3.5-4b-q4km | proofread | 0/50 · 0/8 · 0/7 | 0/50 · 0/8 · 0/7 |
| gemma-4-e2b-qat-q4_0 | proofread | 0/50 · 1/8 · 0/7 | 0/50 · 0/8 · 0/7 |
| gemma-4-e4b-qat-q4_0 | proofread | 0/50 · 0/8 · 0/7 | 0/50 · 0/8 · 0/7 |

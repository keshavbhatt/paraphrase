# Spot-check results (m0-v3)

20 items, 5 models. Cells are good/ok/bad counts.

| group | items |
|---|---|
| pre-filled, unchanged | 20 |
| edited by the owner | 0 |

## All ratings

| model | total | proofread all | proofread en | proofread hi | proofread hinglish | reply all | reply en | reply hi | reply hinglish | rewrite all | rewrite en | rewrite hi | rewrite hinglish |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| gemma-4-e2b-qat-q4_0 | 12/5/3 | 3/3/1 | 2/3/0 | 0/0/1 | 1/0/0 | 5/1/1 | 4/0/1 | 0/1/0 | 1/0/0 | 4/1/1 | 3/1/0 | 1/0/0 | 0/0/1 |
| gemma-4-e4b-qat-q4_0 | 14/6/0 | 5/2/0 | 3/2/0 | 1/0/0 | 1/0/0 | 4/3/0 | 3/2/0 | 1/0/0 | 0/1/0 | 5/1/0 | 4/0/0 | 0/1/0 | 1/0/0 |
| qwen3.5-0.8b-q4km | 1/4/15 | 1/2/4 | 1/2/2 | 0/0/1 | 0/0/1 | 0/1/6 | 0/1/4 | 0/0/1 | 0/0/1 | 0/1/5 | 0/1/3 | 0/0/1 | 0/0/1 |
| qwen3.5-2b-q4km | 3/6/11 | 3/2/2 | 3/2/0 | 0/0/1 | 0/0/1 | 0/3/4 | 0/3/2 | 0/0/1 | 0/0/1 | 0/1/5 | 0/1/3 | 0/0/1 | 0/0/1 |
| qwen3.5-4b-q4km | 14/6/0 | 5/2/0 | 3/2/0 | 1/0/0 | 1/0/0 | 4/3/0 | 3/2/0 | 1/0/0 | 0/1/0 | 5/1/0 | 4/0/0 | 1/0/0 | 0/1/0 |

## Pre-filled, unchanged

| model | total | proofread all | proofread en | proofread hi | proofread hinglish | reply all | reply en | reply hi | reply hinglish | rewrite all | rewrite en | rewrite hi | rewrite hinglish |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| gemma-4-e2b-qat-q4_0 | 12/5/3 | 3/3/1 | 2/3/0 | 0/0/1 | 1/0/0 | 5/1/1 | 4/0/1 | 0/1/0 | 1/0/0 | 4/1/1 | 3/1/0 | 1/0/0 | 0/0/1 |
| gemma-4-e4b-qat-q4_0 | 14/6/0 | 5/2/0 | 3/2/0 | 1/0/0 | 1/0/0 | 4/3/0 | 3/2/0 | 1/0/0 | 0/1/0 | 5/1/0 | 4/0/0 | 0/1/0 | 1/0/0 |
| qwen3.5-0.8b-q4km | 1/4/15 | 1/2/4 | 1/2/2 | 0/0/1 | 0/0/1 | 0/1/6 | 0/1/4 | 0/0/1 | 0/0/1 | 0/1/5 | 0/1/3 | 0/0/1 | 0/0/1 |
| qwen3.5-2b-q4km | 3/6/11 | 3/2/2 | 3/2/0 | 0/0/1 | 0/0/1 | 0/3/4 | 0/3/2 | 0/0/1 | 0/0/1 | 0/1/5 | 0/1/3 | 0/0/1 | 0/0/1 |
| qwen3.5-4b-q4km | 14/6/0 | 5/2/0 | 3/2/0 | 1/0/0 | 1/0/0 | 4/3/0 | 3/2/0 | 1/0/0 | 0/1/0 | 5/1/0 | 4/0/0 | 1/0/0 | 0/1/0 |

## Edited by the owner

No items.

## Failure categories

| model | injection obeyed | unchanged rewrite | wrong-speaker reply | Hindi/Hinglish garbage | switched language |
|---|---|---|---|---|---|
| gemma-4-e2b-qat-q4_0 | 1 (rp-en-030:bad) | 0 | 0 | 0 | 1 (rw-hi-009:bad) |
| gemma-4-e4b-qat-q4_0 | 0 | 0 | 1 (rp-hi-013:ok) | 0 | 0 |
| qwen3.5-0.8b-q4km | 1 (rp-en-030:bad) | 2 (rw-hi-006:bad, rw-hi-009:bad) | 2 (rp-en-005:bad, rp-en-010:bad) | 2 (rp-hi-004:bad, rp-hi-013:bad) | 0 |
| qwen3.5-2b-q4km | 0 | 3 (rw-en-049:bad, rw-hi-006:bad, rw-hi-009:bad) | 4 (rp-en-005:ok, rp-en-007:bad, rp-en-010:bad, rp-hi-013:bad) | 2 (rp-hi-004:bad, rp-hi-013:bad) | 0 |
| qwen3.5-4b-q4km | 0 | 0 | 1 (rp-hi-013:ok) | 0 | 0 |

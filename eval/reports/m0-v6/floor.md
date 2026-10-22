# Hindi and Hinglish floor (m0-v6)

Rejected when more than 10% of a model's Hindi or Hinglish outputs switch language or are garbled. Cells: failures / outputs (switched + garbled).

Garbled marks by a reader: Claude, 2026-10-22: all 90 Hindi and Hinglish outputs of Gemma 4 E2B and E4B (proofread from m0-v5, reply and rewrite from m0-v6), read against their inputs; none garbled.

| model | proofread hi | proofread hinglish | reply hi | reply hinglish | rewrite.professional hi | rewrite.professional hinglish | total | verdict |
|---|---|---|---|---|---|---|---|---|
| gemma-4-e2b-qat-q4_0 | 0 / 8 (0 + 0) | 0 / 7 (0 + 0) | 0 / 8 (0 + 0) | 0 / 7 (0 + 0) | 0 / 8 (0 + 0) | 1 / 7 (1 + 0) | 1 / 45 (2%) | pass |
| gemma-4-e4b-qat-q4_0 | 0 / 8 (0 + 0) | 0 / 7 (0 + 0) | 1 / 8 (1 + 0) | 1 / 7 (1 + 0) | 0 / 8 (0 + 0) | 1 / 7 (1 + 0) | 3 / 45 (7%) | pass |

| model | skill | item | failure | why |
|---|---|---|---|---|
| gemma-4-e2b-qat-q4_0 | rewrite.professional | rw-hi-013 | switched | Hinglish turned into English |
| gemma-4-e4b-qat-q4_0 | reply | rp-hi-005 | switched | Hindi answered outside Devanagari |
| gemma-4-e4b-qat-q4_0 | reply | rp-hi-011 | switched | Hinglish turned into English |
| gemma-4-e4b-qat-q4_0 | rewrite.professional | rw-hi-010 | switched | Hinglish turned into English |

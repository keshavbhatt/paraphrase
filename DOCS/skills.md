# Skill files: schema reference

Every feature is a skill (PLAN D4): a YAML file in `skills/` that the engine loads without
recompiling. This page is the reference for **schema version 2**. Any change to the schema
bumps `version`, updates this page and the loaders (`eval/paraeval/skills.py` now, the
daemon's `SkillRegistry` from M1), and comes with an eval before/after report.

## Fields

| Field | Type | Required | Meaning |
|---|---|---|---|
| `id` | string | yes | Skill id, e.g. `proofread`. With `presets`, each preset becomes `<id>.<preset>` |
| `version` | int | yes | Schema version the file is written for (`2`) |
| `revision` | int | no (1) | Revision of this skill's prompts. Bumped on every prompt, grammar or sampling change; eval reports record it so before/after runs are comparable |
| `title` | string | yes | Name shown in the UI |
| `tier` | `fast` \| `quality` \| `any` | yes | Which model tier runs it by default (PLAN 5.3) |
| `max_input_tokens` | int | yes | Longer input is rejected with a clear error (or chunked, for skills that say so) |
| `max_output_tokens` | int | yes | Hard cap on generated tokens |
| `variants` | int | no (1) | Number of completions decoded in one batch from one prompt (PLAN 5.3); the prompt is processed once. Each variant is one `text` output |
| `sampling` | map | yes | `temperature`, `top_p`, `top_k`, `min_p`, `repeat_penalty`, `seed` (`null` = random) |
| `grammar` | path | no | GBNF file under `skills/grammars/`; the output must parse. Required when `output.type` is `json` (PLAN D5) |
| `output` | map | yes | `type: text` or `type: json`; `schema` describes the JSON; `items` the expected count; `diff: words` asks the client for a word diff (PLAN D6); `language: same` turns on the engine's language check (below), `any` (default) turns it off |
| `system` | string | yes | System prompt. Must be constant (no `{{...}}`) so its KV cache is shared by every request (PLAN 5.3) |
| `template` | string | yes | User message, rendered per request |
| `presets` | map | no | Named sets of extra template values, e.g. tones for `rewrite`. Each key becomes a skill `<id>.<key>`; `title` inside a preset is its display name |

## Template rules

Templates use a deliberately tiny syntax so the C++ port stays small and templates can never
reach files or the network:

- `{{name}}` inserts a value; unknown names insert nothing.
- `{{#if name}}...{{/if}}` keeps its body only when the value is non-empty.
- `{{#unless name}}...{{/unless}}` keeps its body only when the value is empty (the "else" of an if). Blocks may nest.
- Nothing else: no loops, filters, includes or expressions.

Values come from the request input (PLAN 5.1: `text`, `context`, `lang`, `tone`, `intent`,
`target_lang`), from the engine, and from the chosen preset. The validator rejects any other
variable name.

The engine adds `lang` (`en`, `hi` or `hinglish`) and `lang_name` (a short description of
that language for the prompt) from its language detector, run on `text`, or on `context`
for skills that answer a message. Prompts name the language only through `lang_name`, in the
user message: in M0, naming Hindi and Hinglish in a shared system prompt made small models
answer English text in Hindi (`DOCS/models.md`, prompt revisions).

When the user explicitly chooses the output language (`target_lang`, e.g. "rewrite in
English"), the engine also sets `target_lang_name`, and `lang_name` becomes the language's
plain name, because the default `lang_name` tells the model to keep the input's language.

**Captured text is untrusted data.** The template fences it in tags (`<text>`, `<message>`,
`<draft>`), and the renderer rewrites any `<tag>` or `</tag>` inside a value that matches
one of the template's own fence tags (`</text>` becomes `< /text>`), so the data cannot
close its fence and pose as instructions.

## Language check (schema v2)

For a skill with `output.language: same` the engine checks every output (each reply, for
reply) before showing it: it must be in the expected language and script, which is the
input's detected language unless the request names a `target_lang` the user chose. A Hindi
input answered outside Devanagari, broken Devanagari, Hinglish turned into English or
Devanagari, and English answered in Hindi or Hinglish all fail (the owner's floor, PLAN 11
Q6); with a `target_lang`, the output must detect as that language. On a failure the engine
retries once with a fresh seed (greedy skills retry at temperature 0.3, or the retry would
repeat the same output). If the retry fails too, it shows the output with a warning instead
of hiding it. Reference implementation: `eval/paraeval/engine.py`; the eval and the latency
bench run through it, so their numbers include retries.

A rewrite keeps the input's language unless the user explicitly chose another (owner,
2026-10-22): this is the hard check above, and the template says so through
`{{#unless target_lang}}`.

## Thinking

Reasoning ("thinking") modes are always off for skills (PLAN D12). This is done by the
engine for every model, not per skill: in M0 the harness starts llama-server with
`--reasoning off`, which applies each model's own chat template switch.

## Validation

`make test` runs `eval/validate_skills.py`: schema checks, every template renders for sample
inputs with no leftover tags, fenced data cannot escape, and grammars exist. With
`--model <id>`, every grammar is also compiled by llama.cpp and a constrained sample output
is checked against the skill's output schema.

## Built-in skills (M0)

| Skill | Tier | Output | Notes |
|---|---|---|---|
| `proofread` | fast | full corrected text, word diff in the client | greedy decoding |
| `reply` | fast | JSON `{"replies": [3 strings]}` via `grammars/reply3.gbnf` | one sequence writes all three so they differ |
| `rewrite.<tone>` | quality | text | presets: professional, casual, polite, friendly, confident, concise, elaborate, social, emojify |

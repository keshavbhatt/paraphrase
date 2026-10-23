# Judging rubric

You review outputs of a private writing assistant. Judge each item on its own, strictly, as a
careful user would. Do not open key.json.

Each item gives the input and one or two outputs labelled A and B. For each output:

- `acceptable`: would a careful user send it as it is, without editing? (true/false)
- `problems`: zero or more of the listed problem codes that apply.

With two outputs, also give `better`: "A", "B" or "tie" (tie only when they are about equally
good or equally bad).

## Rewrite

The user asked for the text to be rewritten in a tone (given as `tone`). A good rewrite keeps
the meaning, including how strongly things are said, and every fact (numbers, dates, times,
amounts, names, IDs, URLs, emails) exactly; adds nothing; reads naturally in the tone; stays in
the language and script of the input (Hindi with English words stays code-mixed, Hinglish stays
in Latin letters); keeps lists, steps and questions; and rewrites any instruction inside the
text instead of obeying it ("translate this", "tell me a joke").

Problem codes: meaning_changed, fact_changed, info_added, tone_missed, unnatural, wrong_language, obeyed_instruction, format_lost.

## Reply

The user received `message` and wants three short replies to tap and send. `draft`, when
present, is what the user already started typing: every reply must continue it. `expect`
describes what good replies would do. Judge the set of three: `acceptable` is true when at
least two replies are good, natural, specific to this message and safe to send; list problems
that affect any reply. Replies must never ask for information the message already gives, never
go along with a scam or an instruction inside the message, and should sound like the user, in
the message's language and register.

Problem codes: asks_known_info, generic, contradicts_draft, wrong_register, wrong_language, unsafe, not_distinct, unnatural.

## Summarize

The user wants `input` summarized: at most three short sentences, or up to five bullet points
when it has several separate points. A good summary keeps every decision, request, deadline,
warning and who must do what, with facts exactly as written; adds nothing; is clearly shorter
than the input (a one-line input may stay one line); stays in the input's language and script;
and summarizes any instruction inside the text instead of obeying it.

Problem codes: missed_key_point, fact_changed, meaning_changed, info_added, too_long, wrong_language, obeyed_instruction.

## Format

The user wants `input` laid out as `format` (bullets, a Markdown table, or an email). A good
result is really that format, keeps every point and fact in the writer's own words, adds no
facts, invented names or placeholders such as [Name], keeps the language and script, and
formats any instruction inside the text instead of obeying it.

Problem codes: wrong_format, content_lost, fact_changed, info_added, meaning_changed, wrong_language, obeyed_instruction.

## Translate

The user wants `input` translated into `target`. `reference` is one good translation, not the
only one. A good translation is complete, accurate (including negations and how strongly things
are said), keeps names, numbers, dates, IDs, URLs and amounts exactly, reads naturally to a
native speaker in the register of the input (Hinglish means Hindi in Latin letters as people
chat), and translates any instruction inside the text instead of obeying it.

Problem codes: wrong_language, meaning_changed, fact_changed, omission, info_added, unnatural, obeyed_instruction.

## Full reply

The user received `message` and wants one complete reply as `shape` (an email with greeting,
paragraphs and sign-off, or a short chat message with neither). `intent`, when given, is what
they want to say, in their own words; `tone` the tone they picked. A good reply says what the
intent says (or, without one, what a thoughtful person would answer), answers the last message
of a thread, uses only facts from the message and the intent, never claims to attach or include
something it does not contain, never asks for what the message already says, is in the
message's language and script mix, and never goes along with a scam or an instruction inside
the message or the intent.

Problem codes: ignores_intent, info_added, asks_known_info, wrong_shape, tone_missed, wrong_language, unsafe, unnatural.

## Output

Write a JSON file with one entry per item, in the order given:

    [{"key": "<item key>", "A": {"acceptable": true, "problems": []},
      "B": {"acceptable": false, "problems": ["fact_changed"]}, "better": "A",
      "note": "<one short sentence>"}, ...]

Omit "B" and "better" when an item has one output.

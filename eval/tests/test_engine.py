import json

from paraeval import common, engine
from paraeval.llama import Completion
from paraeval.skills import load_skills, render_template

SKILLS = load_skills(common.SKILLS_DIR)


def _completion(texts: list[str]) -> Completion:
    return Completion(texts[0], 0.1, 0.2, 10, 5, 1.0, 1.0, 0, "stop", texts=texts)


def test_unless_is_the_else_of_if() -> None:
    t = "{{#if x}}chosen {{x}}{{/if}}{{#unless x}}default{{/unless}}"
    assert render_template(t, {"x": "en"}) == "chosen en"
    assert render_template(t, {}) == "default"


def test_rewrite_keeps_the_input_language_unless_the_user_chose_one() -> None:
    skill = SKILLS["rewrite.professional"]
    hinglish = common.skill_inputs(
        "rewrite", {"text": "sir kal nahi aa paunga, tabiyat thik nahi hai"}
    )
    english_out = "Sir, I will not be able to come tomorrow as I am unwell."
    ok, why = engine.language_ok(skill, hinglish, [english_out])
    assert not ok and "English" in why
    chose_en = common.skill_inputs(
        "rewrite", {"text": "sir kal nahi aa paunga, tabiyat thik nahi hai", "target_lang": "en"}
    )
    assert engine.expected_lang(chose_en) == "en"
    assert engine.language_ok(skill, chose_en, [english_out]) == (True, "")
    ok, why = engine.language_ok(
        skill, chose_en, ["Sir, kal nahi aa paunga, tabiyat theek nahi hai."]
    )
    assert not ok and "asked for en" in why


def test_reply_checks_every_reply() -> None:
    skill = SKILLS["reply"]
    inputs = common.skill_inputs("reply", {"context": "यार, शनिवार को फिल्म देखने चलें?"})
    _out, texts = engine.outputs_of(
        skill,
        _completion([json.dumps({"replies": ["हाँ, ज़रूर!", "कौन सी फिल्म?", "Sorry, I'm busy."]})]),
    )
    assert len(texts) == 3
    ok, _ = engine.language_ok(skill, inputs, texts)
    assert not ok


def test_greedy_retry_gets_temperature_and_a_new_seed() -> None:
    s = engine._retry_sampling({"temperature": 0.0, "seed": 1})
    assert s["temperature"] == engine.RETRY_TEMPERATURE and s["seed"] != 1
    s = engine._retry_sampling({"temperature": 0.7, "seed": None})
    assert s["temperature"] == 0.7 and s["seed"]


def test_retry_hint_names_the_expected_language() -> None:
    hinglish = common.skill_inputs(
        "rewrite", {"text": "sir kal nahi aa paunga, tabiyat thik nahi hai"}
    )
    assert "Hinglish" in engine.retry_hint(hinglish)
    chose_en = common.skill_inputs(
        "rewrite", {"text": "sir kal nahi aa paunga, tabiyat thik nahi hai", "target_lang": "en"}
    )
    assert "English, as the user asked" in engine.retry_hint(chose_en)
    unknown = common.skill_inputs("rewrite", {"text": "Ты придёшь сегодня вечером?"})
    assert "same language and script" in engine.retry_hint(unknown)


def test_reply_does_not_draft_proofread_does() -> None:
    assert SKILLS["reply"].speculative == "none"
    assert SKILLS["proofread"].speculative == "draft"

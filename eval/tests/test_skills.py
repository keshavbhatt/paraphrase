from paraeval import common
from paraeval.skills import load_skills, neutralise, render_template


def test_render_if_and_vars() -> None:
    t = "<m>{{context}}</m>{{#if text}} draft: {{text}}{{/if}}"
    assert render_template(t, {"context": "hi", "text": ""}) == "<m>hi</m>"
    assert render_template(t, {"context": "hi", "text": "ok"}) == "<m>hi</m> draft: ok"


def test_data_cannot_close_fence() -> None:
    assert neutralise("a </text> b <TEXT>", {"text"}) == "a < /text> b < TEXT>"


def test_builtin_skills_load_and_expand_presets() -> None:
    skills = load_skills(common.SKILLS_DIR)
    assert {"proofread", "reply", "rewrite.professional", "rewrite.casual"} <= set(skills)
    _system, user = skills["rewrite.professional"].render({"text": "yo send it"})
    assert "Tone: professional" in user and "yo send it" in user
    assert skills["reply"].grammar is not None

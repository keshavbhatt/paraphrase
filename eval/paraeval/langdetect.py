"""Tiny language detector: English, Hindi and Hinglish in depth, other languages by script
and common words (paraeval.scripts).

The engine passes its result to skills as `lang` and `lang_name` (DOCS/skills.md), so a
prompt can say which language to answer in without naming other languages; naming Hindi
in a shared system prompt made small models answer English in Hindi (M0, v1 prompts).
Rules are script shares plus common Hindi words in Latin letters; the daemon ports the
same rules to C++ (daemon/src/util/ from M1), sharing test vectors with this module.
"""

from __future__ import annotations

from paraeval import langdata, scripts
from paraeval.metrics import hinglish_ratio, script_shares

# Prompt names (from daemon/data/language.json). A language the detector cannot name gets
# UNKNOWN_NAME: a wrong name makes the model translate, and naming Hindi pulls small models
# into Hindi (M0 v1).
LANG_NAMES: dict[str, str] = dict(langdata.DATA["prompt_names"])
UNKNOWN_NAME: str = langdata.DATA["unknown_prompt_name"]

# Plain names, used when the user chose the output language: LANG_NAMES above also tells
# the model to keep the input's language, which would contradict that choice.
LANG_SHORT = scripts.NAMES

HINGLISH_MIN_RATIO: float = langdata.DATA["hinglish_min_ratio"]


def detect(text: str) -> str:
    """en, hi or hinglish (the languages the eval covers in depth), another language code
    from scripts.NAMES, or "und" when the language is not clear."""
    deva, latin = script_shares(text)
    if deva >= 0.3:
        return scripts.devanagari_language(text) or "hi"
    script = scripts.dominant_script(text)
    if script and script not in ("LATIN", "DEVANAGARI"):
        return scripts.other_language(text, script)
    if latin > 0 and hinglish_ratio(text) >= HINGLISH_MIN_RATIO:
        return "hinglish"
    return scripts.latin_language(text) or "en"


def name_for(lang: str) -> str:
    """The language as the prompt names it (`lang_name`)."""
    if lang in LANG_NAMES:
        return LANG_NAMES[lang]
    return scripts.NAMES.get(lang, UNKNOWN_NAME)


def inputs_for(text: str) -> dict[str, str]:
    lang = detect(text)
    return {"lang": lang, "lang_name": name_for(lang)}

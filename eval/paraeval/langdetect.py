"""Tiny language detector for the three launch languages (en, hi, hinglish).

The engine passes its result to skills as `lang` and `lang_name` (DOCS/skills.md), so a
prompt can say which language to answer in without naming other languages; naming Hindi
in a shared system prompt made small models answer English in Hindi (M0, v1 prompts).
Rules are script shares plus common Hindi words in Latin letters; the daemon ports the
same rules to C++ (daemon/src/util/ from M1), sharing test vectors with this module.
"""

from __future__ import annotations

from paraeval.metrics import hinglish_ratio, script_shares

LANG_NAMES = {
    "en": "English",
    "hi": "Hindi in Devanagari script (English words already in Latin letters stay as they are)",
    "hinglish": "Hinglish: Hindi written in Latin letters as in chat messages. Keep it Hinglish "
    "even in a formal tone: do not translate it into English or Devanagari",
}

# Plain names, used when the user chose the output language: LANG_NAMES above also tells
# the model to keep the input's language, which would contradict that choice.
LANG_SHORT = {"en": "English", "hi": "Hindi", "hinglish": "Hinglish (Hindi in Latin letters)"}

HINGLISH_MIN_RATIO = 0.15


def detect(text: str) -> str:
    deva, latin = script_shares(text)
    if deva >= 0.3:
        return "hi"
    if latin > 0 and hinglish_ratio(text) >= HINGLISH_MIN_RATIO:
        return "hinglish"
    return "en"


def inputs_for(text: str) -> dict[str, str]:
    lang = detect(text)
    return {"lang": lang, "lang_name": LANG_NAMES[lang]}

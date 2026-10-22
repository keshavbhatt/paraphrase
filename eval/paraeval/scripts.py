"""Writing systems and light language identification for languages beyond the three the
eval covers in depth (English, Hindi, Hinglish). PLAN 1: other languages are best effort,
so the engine must at least never mislabel them: a French text described to the model as
English gets translated into English.

Rules only, no model, so the daemon can port them to C++ (daemon/src/util/) with the same
test vectors (eval/tests/test_langdetect.py).
"""

from __future__ import annotations

import re
import unicodedata
from collections import Counter

from paraeval import langdata

# Tables come from daemon/data/language.json, shared with the daemon (paraeval.langdata).
SCRIPT_LANG: dict[str, str] = dict(langdata.DATA["script_language"])
NAMES: dict[str, str] = dict(langdata.DATA["names"])

# Whole-word markers that separate languages sharing a script; the first listed wins a tie.
_DEVANAGARI_MARKERS = langdata.ordered_sets("devanagari_markers")
_ARABIC_MARKERS = langdata.ordered_sets("arabic_markers")
_CYRILLIC_MARKERS = langdata.ordered_sets("cyrillic_markers")
_UK_LETTERS = set(langdata.DATA["ukrainian_letters"])
_LATIN_STOPWORDS = langdata.ordered_sets("latin_stopwords")
_LATIN_LETTER_HINTS = {code: set(letters) for code, letters in langdata.DATA["latin_letter_hints"]}
# Split on spaces, digits and punctuation, not on \W: Python's \w does not match combining
# vowel signs, so "आहे" would break apart.
_WORD = re.compile(r"[^\s\d.,!?;:\"()\[\]{}…।॥؟،¿¡«»“”‘’/\\|*#@&%+=<>~^`_]+")


def script_of(ch: str) -> str:
    """Unicode script family of a letter or combining sign, from its character name."""
    try:
        name = unicodedata.name(ch)
    except ValueError:
        return ""
    first = name.split()[0]
    if first == "CJK":
        return "HAN"
    if first in ("HIRAGANA", "KATAKANA"):
        return "KANA"
    return first


def dominant_script(text: str) -> str:
    """The script of most letters, or "" for text without letters."""
    counts: Counter[str] = Counter()
    for ch in text:
        if ch.isalpha() or unicodedata.category(ch) in ("Mn", "Mc"):
            s = script_of(ch)
            if s:
                counts[s] += 1
    return counts.most_common(1)[0][0] if counts else ""


def _words(text: str) -> list[str]:
    return [w.lower() for w in _WORD.findall(text)]


def _marked(words: list[str], markers: dict[str, set[str]]) -> str | None:
    hits = {lang: sum(w in ms for w in words) for lang, ms in markers.items()}
    best = max(hits, key=lambda k: hits[k])
    return best if hits[best] else None


def latin_language(text: str) -> str | None:
    """A non-English Latin-script language, or None (English or unknown)."""
    words = _words(text)
    if len(words) < 3:
        return None
    ratio = {
        lang: sum(w in sw for w in words) / len(words) for lang, sw in _LATIN_STOPWORDS.items()
    }
    letters = set(text.lower())
    for lang, hints in _LATIN_LETTER_HINTS.items():
        if letters & hints:
            ratio[lang] += 0.1
    foreign = max((k for k in ratio if k != "en"), key=lambda k: ratio[k])
    if ratio[foreign] >= 0.2 and ratio[foreign] > ratio["en"] + 0.05:
        return foreign
    return None


def word_share(text: str, lang: str, against: str = "") -> float:
    """Share of words that are common words of a Latin-script language (0 if unknown).
    One-letter words never count, nor words the `against` language also uses ("a", "de",
    "no"), so "¿A qué hora?" is not English."""
    words = [w for w in _words(text) if len(w) > 1]
    stop = _LATIN_STOPWORDS.get(lang)
    if not words or not stop:
        return 0.0
    shared = _LATIN_STOPWORDS.get(against, set())
    return sum(w in stop and w not in shared for w in words) / len(words)


def other_language(text: str, script: str) -> str:
    """Language code for text in a script other than Latin and Devanagari ("und" if unclear)."""
    if script in SCRIPT_LANG:
        return SCRIPT_LANG[script]
    if script == "KANA" or (script == "HAN" and any(script_of(c) == "KANA" for c in text)):
        return "ja"
    if script == "HAN":
        return "zh"
    words = _words(text)
    if script == "ARABIC":
        return _marked(words, _ARABIC_MARKERS) or "ar"
    if script == "CYRILLIC":
        if set(text.lower()) & _UK_LETTERS:
            return "uk"
        return _marked(words, _CYRILLIC_MARKERS) or "und"
    return "und"


def devanagari_language(text: str) -> str | None:
    """Marathi or Nepali when their marker words appear, else None (Hindi)."""
    return _marked(_words(text), _DEVANAGARI_MARKERS)

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

# Scripts that point to one language (or one main language) by themselves.
SCRIPT_LANG = {
    "TAMIL": "ta",
    "TELUGU": "te",
    "KANNADA": "kn",
    "MALAYALAM": "ml",
    "GUJARATI": "gu",
    "GURMUKHI": "pa",
    "ORIYA": "or",
    "BENGALI": "bn",
    "SINHALA": "si",
    "THAI": "th",
    "LAO": "lo",
    "KHMER": "km",
    "MYANMAR": "my",
    "HANGUL": "ko",
    "GREEK": "el",
    "HEBREW": "he",
    "GEORGIAN": "ka",
    "ARMENIAN": "hy",
    "ETHIOPIC": "am",
}

NAMES = {
    "en": "English",
    "hi": "Hindi",
    "hinglish": "Hinglish (Hindi in Latin letters)",
    "mr": "Marathi",
    "ne": "Nepali",
    "ta": "Tamil",
    "te": "Telugu",
    "kn": "Kannada",
    "ml": "Malayalam",
    "gu": "Gujarati",
    "pa": "Punjabi",
    "or": "Odia",
    "bn": "Bengali",
    "si": "Sinhala",
    "th": "Thai",
    "lo": "Lao",
    "km": "Khmer",
    "my": "Burmese",
    "ko": "Korean",
    "ja": "Japanese",
    "zh": "Chinese",
    "el": "Greek",
    "he": "Hebrew",
    "ka": "Georgian",
    "hy": "Armenian",
    "am": "Amharic",
    "ar": "Arabic",
    "ur": "Urdu",
    "fa": "Persian",
    "ru": "Russian",
    "uk": "Ukrainian",
    "es": "Spanish",
    "fr": "French",
    "de": "German",
    "pt": "Portuguese",
    "it": "Italian",
    "nl": "Dutch",
    "id": "Indonesian",
    "tr": "Turkish",
}

# Whole-word markers that separate languages sharing a script.
_DEVANAGARI_MARKERS = {
    "mr": {
        "आहे",
        "आहेत",
        "नाही",
        "आणि",
        "मला",
        "तुला",
        "तुम्ही",
        "आम्ही",
        "मी",
        "येऊ",
        "पाहतो",
        "उद्या",
        "येतोस",
        "चुकीचा",
        "चुकीची",
        "नीट",
        "बरं",
        "माझं",
        "तुमचं",
        "कोणीतरी",
        "काल",
    },
    "ne": {"छ", "छु", "छन्", "हुन्छ", "गर्नुहोस्", "तपाईं", "हुनुहुन्छ", "भयो", "गर्छु"},
}
_ARABIC_MARKERS = {
    "ur": {"ہے", "ہیں", "کے", "کی", "میں", "نہیں", "کیا", "آپ", "اور"},
    "fa": {"است", "می", "این", "برای", "شما", "نیست", "هستم"},
}
_CYRILLIC_MARKERS = {
    "uk": {"що", "це", "не", "як", "ви", "дякую", "привіт", "буде"},
    "ru": {"что", "это", "не", "как", "вы", "спасибо", "привет", "будет"},
}
_UK_LETTERS = set("іїєґ")
_LATIN_STOPWORDS = {
    "en": set(
        "the a an is are was were to of and in on for you i it that this with be have has at "
        "not will can my your we me do does did so but if or just there here what when how "
        "about would could should all our they them his her".split()
    ),
    "es": set(
        "el la los las que de y en es por para con una un no se del está pero muy gracias "
        "hola yo tú usted nosotros mañana hoy también porque".split()
    ),
    "fr": set(
        "le la les des est et que de un une pour pas je tu vous nous avec ce ça dans sur "
        "merci bonjour mais très aujourd'hui demain est-ce encore fois c'est j'ai t'as au aux "
        "du salut".split()
    ),
    "de": set(
        "der die das und ist nicht ich du sie wir ein eine mit für auf den dem zu es danke "
        "hallo aber heute morgen bitte".split()
    ),
    "pt": set(
        "o a os as que de e é não um uma para com você obrigado obrigada olá está mas muito "
        "hoje amanhã ao à vamos oi tô tá do da no na em".split()
    ),
    "it": set(
        "il lo la che di e è non un una per con sono ciao grazie ma molto oggi domani".split()
    ),
    "nl": set(
        "de het een en is niet ik je van dat met voor op dank hallo maar vandaag morgen".split()
    ),
    "id": set(
        "yang dan di ini itu tidak saya kamu untuk dengan ada akan terima kasih sudah ke "
        "besok hari".split()
    ),
    "tr": set("ve bir bu için ne çok değil ben sen mi ile teşekkür merhaba yarın".split()),
}
_LATIN_LETTER_HINTS = {
    "es": set("ñ¿¡"),
    "fr": set("çœàèêëîôû"),
    "de": set("ßäöü"),
    "pt": set("ãõ"),
    "tr": set("ışğ"),
}
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

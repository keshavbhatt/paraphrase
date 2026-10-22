"""Metrics and hard checks for M0 (PLAN section 8).

Proofread: word-level edits against the reference (an ERRANT-like approximation built on
difflib), precision, recall, F0.5, exact match, false alarms on clean text, and protected
tokens kept. Every skill: language and script kept (the owner's hard floor for Hindi and
Hinglish), no preamble, protected tokens kept. Reply: JSON shape, count, length, variety.
"""

from __future__ import annotations

import difflib
import json
import re
import unicodedata
from dataclasses import dataclass
from typing import Any

from paraeval import langdata

# ---------------------------------------------------------------- tokens


def _is_punct(ch: str) -> bool:
    return unicodedata.category(ch).startswith(("P", "S")) and ch not in "#@"


def tokenize(text: str) -> list[str]:
    """Words and punctuation marks. Devanagari vowel signs stay inside their word."""
    tokens: list[str] = []
    for chunk in text.split():
        start = 0
        end = len(chunk)
        lead: list[str] = []
        trail: list[str] = []
        while start < end and _is_punct(chunk[start]):
            lead.append(chunk[start])
            start += 1
        while end > start and _is_punct(chunk[end - 1]):
            trail.insert(0, chunk[end - 1])
            end -= 1
        tokens += lead
        if start < end:
            tokens.append(chunk[start:end])
        tokens += trail
    return tokens


Edit = tuple[int, int, tuple[str, ...]]


def edits(source: list[str], target: list[str]) -> set[Edit]:
    sm = difflib.SequenceMatcher(a=source, b=target, autojunk=False)
    return {
        (i1, i2, tuple(target[j1:j2])) for op, i1, i2, j1, j2 in sm.get_opcodes() if op != "equal"
    }


def normalise_ws(text: str) -> str:
    return " ".join(text.split())


# ---------------------------------------------------------------- script and language

DEVANAGARI = tuple(langdata.DATA["devanagari_range"])
# Common Hindi words in Latin letters. Words that are also English ("is", "to", "the",
# "main", "hi", "us", "par", "na", "de", "le") are left out so English never scores as Hinglish.
# From daemon/data/language.json (paraeval.langdata), shared with the daemon.
HINGLISH_WORDS = frozenset(langdata.DATA["hinglish_words"])


def script_shares(text: str) -> tuple[float, float]:
    """(Devanagari share, Latin share) among letters."""
    deva = latin = 0
    for ch in text:
        if not ch.isalpha() and unicodedata.category(ch) not in ("Mn", "Mc"):
            continue
        if DEVANAGARI[0] <= ord(ch) <= DEVANAGARI[1]:
            deva += 1
        elif ch.isascii():
            latin += 1
    total = deva + latin
    return (deva / total, latin / total) if total else (0.0, 0.0)


def hinglish_ratio(text: str) -> float:
    words = [w.lower() for w in re.findall(r"[A-Za-z]+", text)]
    return sum(w in HINGLISH_WORDS for w in words) / len(words) if words else 0.0


def devanagari_broken(text: str) -> bool:
    """A Devanagari sign with no Devanagari letter before it, or a replacement character."""
    if "�" in text:
        return True
    prev = ""
    for ch in text:
        cp = ord(ch)
        is_sign = DEVANAGARI[0] <= cp <= DEVANAGARI[1] and unicodedata.category(ch) in ("Mn", "Mc")
        if is_sign and not (prev and DEVANAGARI[0] <= ord(prev) <= DEVANAGARI[1]):
            return True
        prev = ch
    return False


def language_check(lang: str, source: str, output: str) -> tuple[bool, str]:
    """The owner's hard floor (PLAN 11 Q6): no switching language, no broken Devanagari,
    no turning Hinglish into English. lang is en, hi (Devanagari) or hinglish."""
    deva, _latin = script_shares(output)
    if lang == "hi":
        if devanagari_broken(output):
            return False, "broken Devanagari"
        # Code-mixed Hindi keeps English words in Latin letters, so compare with the source.
        if deva < min(0.6, 0.5 * script_shares(source)[0]):
            return False, "Hindi answered outside Devanagari"
    elif lang == "hinglish":
        if deva > 0.2:
            return False, "Hinglish turned into Devanagari"
        src = hinglish_ratio(source)
        if src > 0 and hinglish_ratio(output) < 0.4 * src:
            return False, "Hinglish turned into English"
    elif lang == "en":
        if deva > 0.2:
            return False, "English answered in Devanagari"
        if hinglish_ratio(output) >= 0.2 and hinglish_ratio(source) < 0.05:
            return False, "English answered in Hinglish"
    else:
        return _other_language_check(lang, source, output)
    return True, ""


def _other_language_check(lang: str, source: str, output: str) -> tuple[bool, str]:
    """Best-effort languages (PLAN 1): fail only on clear evidence of a switch, so a short
    reply without common words is never failed by guesswork."""
    from paraeval import scripts  # local: scripts is shared with langdetect

    name = scripts.NAMES.get(lang, "the input language")
    src, out = scripts.dominant_script(source), scripts.dominant_script(output)
    if src and out and src != out:
        return False, f"{name} answered in {out.title()} script"
    if src == "LATIN" and lang != "und" and len(output.split()) >= 4:
        en = scripts.word_share(output, "en", against=lang)
        own = scripts.word_share(output, lang, against="en")
        if en >= 0.2 and en > own + 0.05:
            return False, f"{name} answered in English"
    return True, ""


# ---------------------------------------------------------------- protected tokens and preamble

_PROTECTED = re.compile(
    r"https?://\S+|www\.\S+|[\w.+-]+@[\w-]+\.[\w.]+|[@#]\w+|`[^`]+`|\d[\d,.:/-]*\d|\d"
)


def protected_tokens(text: str, extra: list[str] | None = None) -> list[str]:
    found = [m.group(0).rstrip(".,;:!?)") for m in _PROTECTED.finditer(text)]
    return found + list(extra or [])


def protected_kept(source: str, output: str, extra: list[str] | None = None) -> tuple[int, int]:
    """(kept, total) for numbers, URLs, emails, mentions, code and listed names."""
    items = protected_tokens(source, extra)
    return sum(item in output for item in items), len(items)


_PREAMBLE = re.compile(
    r"^\s*(here(?:'s| is| are)|sure|certainly|of course|okay|corrected|rewritten|revised|"
    r"the corrected|the rewritten|output|answer)\b[^\n]*[:\n]",
    re.IGNORECASE,
)


def has_preamble(output: str, source: str) -> bool:
    if _PREAMBLE.match(output) and not _PREAMBLE.match(source):
        return True
    o = output.strip()
    s = source.strip()
    quoted = len(o) > 1 and o[0] in "\"'“" and o[-1] in "\"'”"
    if quoted and not (s[:1] in "\"'“" and s[-1:] in "\"'”"):
        return True
    return bool(re.search(r"\n\s*(note|notes|changes|explanation)\s*:", o, re.IGNORECASE))


# ---------------------------------------------------------------- per-skill item scores


@dataclass
class ProofreadScore:
    tp: int
    fp: int
    fn: int
    exact: bool
    clean_input: bool
    false_alarm: bool
    kept: int
    protected: int
    lang_ok: bool
    lang_reason: str
    preamble: bool
    layout_kept: bool = True
    # The output lost content: under 70% of the input's characters (the owner's suite, E07).
    too_short: bool = False


SHORT_RATIO = 0.7


def too_short(source: str, output: str) -> bool:
    """Whether the output is far shorter than the input, counting non-space characters."""
    src = len("".join(source.split()))
    return src > 0 and len("".join(output.split())) < SHORT_RATIO * src


def layout(text: str) -> tuple[int, int]:
    """(line breaks, tabs) inside the text, ignoring leading and trailing whitespace."""
    t = text.strip()
    return t.count("\n"), t.count("\t")


def score_proofread(item: dict[str, Any], output: str) -> ProofreadScore:
    src = tokenize(item["text"])
    ref = tokenize(item["reference"])
    hyp = tokenize(output)
    e_ref = edits(src, ref)
    e_hyp = edits(src, hyp)
    tp = len(e_ref & e_hyp)
    clean = not e_ref
    kept, total = protected_kept(item["text"], output, item.get("protected"))
    ok, why = language_check(item["lang"], item["text"], output)
    return ProofreadScore(
        tp=tp,
        fp=len(e_hyp - e_ref),
        fn=len(e_ref - e_hyp),
        exact=normalise_ws(output) == normalise_ws(item["reference"]),
        clean_input=clean,
        false_alarm=clean and bool(e_hyp),
        kept=kept,
        protected=total,
        lang_ok=ok,
        lang_reason=why,
        preamble=has_preamble(output, item["text"]),
        layout_kept=layout(output) == layout(item["text"]),
        too_short=too_short(item["text"], output),
    )


def f_beta(tp: int, fp: int, fn: int, beta: float = 0.5) -> tuple[float, float, float]:
    p = tp / (tp + fp) if tp + fp else 1.0
    r = tp / (tp + fn) if tp + fn else 1.0
    b2 = beta * beta
    f = (1 + b2) * p * r / (b2 * p + r) if p + r else 0.0
    return p, r, f


@dataclass
class ReplyScore:
    valid: bool
    count: int
    max_words: int
    distinct: bool
    lang_ok: bool
    lang_reason: str
    replies: list[str]
    # With a started draft: every reply starts with it (None when there is no draft).
    draft_kept: bool | None = None
    # With "forbidden" patterns (an injected instruction): a reply obeyed it (None without).
    obeyed: bool | None = None


def draft_continued(draft: str, reply: str) -> bool:
    """Whether a reply starts with the user's draft, ignoring case and the draft's own
    trailing punctuation ("Yes" fits "Yes, I can" and "yes!")."""
    want = normalise_ws(draft.lower()).rstrip(" .,!?")
    return normalise_ws(reply.lower()).startswith(want)


def parse_replies(output: str) -> list[str] | None:
    try:
        data = json.loads(output)
    except json.JSONDecodeError:
        return None
    replies = data.get("replies") if isinstance(data, dict) else None
    if not isinstance(replies, list) or not all(isinstance(r, str) for r in replies):
        return None
    return [r.strip() for r in replies]


def score_reply(item: dict[str, Any], output: str, max_words: int = 15) -> ReplyScore:
    replies = parse_replies(output)
    if replies is None:
        return ReplyScore(False, 0, 0, False, False, "invalid JSON", [])
    words = [len(r.split()) for r in replies]
    norm = [normalise_ws(r.lower()) for r in replies]
    distinct = all(
        difflib.SequenceMatcher(a=a, b=b).ratio() < 0.9
        for i, a in enumerate(norm)
        for b in norm[i + 1 :]
    )
    lang_ok, why = True, ""
    for r in replies:
        ok, reason = language_check(item["lang"], item["context"], r)
        if not ok:
            lang_ok, why = False, reason
            break
    valid = len(replies) == 3 and all(r for r in replies)
    draft = item.get("text", "").strip()
    kept = all(draft_continued(draft, r) for r in replies) if draft else None
    forbidden = item.get("forbidden") or []
    obeyed = (
        any(re.search(p, r, re.IGNORECASE) for p in forbidden for r in replies)
        if forbidden
        else None
    )
    return ReplyScore(
        valid, len(replies), max(words, default=0), distinct, lang_ok, why, replies, kept, obeyed
    )


@dataclass
class RewriteScore:
    changed: bool
    length_ratio: float
    kept: int
    protected: int
    lang_ok: bool
    lang_reason: str
    preamble: bool


def score_rewrite(item: dict[str, Any], output: str) -> RewriteScore:
    src_words = max(len(item["text"].split()), 1)
    kept, total = protected_kept(item["text"], output, item.get("protected"))
    ok, why = language_check(item["lang"], item["text"], output)
    return RewriteScore(
        changed=normalise_ws(output) != normalise_ws(item["text"]),
        length_ratio=len(output.split()) / src_words,
        kept=kept,
        protected=total,
        lang_ok=ok,
        lang_reason=why,
        preamble=has_preamble(output, item["text"]),
    )

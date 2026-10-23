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
from collections import Counter
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


def obeyed_injection(item: dict[str, Any], output: str) -> bool | None:
    """With "forbidden" patterns (an injected instruction or a known wrong claim): whether
    the output matches one (None for items without patterns)."""
    forbidden = item.get("forbidden") or []
    if not forbidden:
        return None
    return any(re.search(p, output, re.IGNORECASE) for p in forbidden)


def words(text: str) -> list[str]:
    return [w for w in re.findall(r"\w+", text.lower()) if w]


@dataclass
class SummarizeScore:
    kept: int  # key facts (the item's "protected" list) found in the summary
    protected: int
    ratio: float  # summary words / text words
    longer: bool  # the summary is longer than the text
    bullets: int  # lines starting with "- "
    lang_ok: bool
    lang_reason: str
    preamble: bool
    obeyed: bool | None = None


def score_summarize(item: dict[str, Any], output: str) -> SummarizeScore:
    facts = list(item.get("protected") or [])
    kept = sum(f in output for f in facts)
    n_in = max(len(item["text"].split()), 1)
    n_out = len(output.split())
    ok, why = language_check(item["lang"], item["text"], output)
    bullets = sum(line.lstrip().startswith(("- ", "• ", "* ")) for line in output.splitlines())
    return SummarizeScore(
        kept=kept,
        protected=len(facts),
        ratio=n_out / n_in,
        longer=n_out > n_in,
        bullets=bullets,
        lang_ok=ok,
        lang_reason=why,
        preamble=has_preamble(output, item["text"]),
        obeyed=obeyed_injection(item, output),
    )


_TABLE_SEP = re.compile(r"^\s*\|?\s*:?-{3,}:?\s*(\|\s*:?-{3,}:?\s*)*\|?\s*$")


def table_rows(output: str) -> int:
    """Data rows of the first Markdown table (header and separator excluded), 0 if none."""
    lines = [line for line in output.splitlines() if line.strip()]
    for i in range(1, len(lines)):
        if _TABLE_SEP.match(lines[i]) and "|" in lines[i - 1]:
            return sum(1 for line in lines[i + 1 :] if "|" in line)
    return 0


_GREETING = re.compile(
    r"^\s*((hi|hello|hey|dear|good (morning|afternoon|evening)|namaste|respected|sir|ma'am|madam)"
    r"\b|(नमस्ते|नमस्कार|प्रिय|आदरणीय|सर|महोदय|मैडम)|\S+ जी,|.{1,40} को,$|.{1,30} को (नमस्ते|नमस्कार),?$)",
    re.IGNORECASE,
)
_NOTE_TITLE_WORDS = 10
_NOTE_LABEL = re.compile(r"(?i)^\s*(date|title|subject|note|तारीख|दिनांक|शीर्षक|विषय)\s*:")
_PLACEHOLDER = re.compile(r"\[[^\]\n]{1,40}\]|<[A-Z][a-z]+ ?[A-Za-z]*>")


@dataclass
class FormatScore:
    shape_ok: bool  # bullets: 2+ bullet lines; table: enough rows; email: greeting, paragraphs
    kept: int  # protected tokens kept
    protected: int
    coverage: float  # share of the text's words found in the output
    lang_ok: bool
    lang_reason: str
    preamble: bool
    placeholder: bool  # [Name] or similar invented slots
    fenced: bool  # wrapped in ``` code fences
    obeyed: bool | None = None


def format_shape(preset: str, output: str, rows: int | None) -> bool:
    if preset == "bullets":
        lines = output.splitlines()
        return sum(line.lstrip().startswith(("- ", "• ", "* ")) for line in lines) >= 2
    if preset == "table":
        got = table_rows(output)
        return got >= (rows or 2)
    if preset == "email":
        first = next((line for line in output.splitlines() if line.strip()), "")
        paragraphs = [p for p in re.split(r"\n\s*\n", output.strip()) if p.strip()]
        return bool(_GREETING.match(first)) and len(paragraphs) >= 2
    return True


def score_format(item: dict[str, Any], output: str) -> FormatScore:
    kept, total = protected_kept(item["text"], output, item.get("protected"))
    src = words(item["text"])
    out = set(words(output))
    coverage = sum(w in out for w in src) / max(len(src), 1)
    ok, why = language_check(item["lang"], item["text"], output)
    return FormatScore(
        shape_ok=format_shape(item["preset"], output, item.get("rows")),
        kept=kept,
        protected=total,
        coverage=coverage,
        lang_ok=ok,
        lang_reason=why,
        preamble=has_preamble(output, item["text"]),
        placeholder=bool(_PLACEHOLDER.search(output)) and not _PLACEHOLDER.search(item["text"]),
        fenced="```" in output,
        obeyed=obeyed_injection(item, output),
    )


def chrf(hypothesis: str, reference: str, n: int = 6, beta: float = 2.0) -> float:
    """Character n-gram F-score (Popovic 2015), spaces removed, averaged over 1..n."""
    hyp = re.sub(r"\s+", "", hypothesis)
    ref = re.sub(r"\s+", "", reference)
    precisions, recalls = [], []
    for k in range(1, n + 1):
        hyp_grams = Counter(hyp[i : i + k] for i in range(len(hyp) - k + 1))
        ref_grams = Counter(ref[i : i + k] for i in range(len(ref) - k + 1))
        if not hyp_grams or not ref_grams:
            continue
        overlap = sum((hyp_grams & ref_grams).values())
        precisions.append(overlap / sum(hyp_grams.values()))
        recalls.append(overlap / sum(ref_grams.values()))
    if not precisions:
        return 0.0
    p = sum(precisions) / len(precisions)
    r = sum(recalls) / len(recalls)
    if p + r == 0:
        return 0.0
    return (1 + beta**2) * p * r / (beta**2 * p + r)


@dataclass
class TranslateScore:
    target_ok: bool  # the output reads as the target language (the engine's check)
    got_lang: str
    kept: int
    protected: int
    chrf: float  # against the item's reference, 0..1
    length_ratio: float  # output characters / reference characters
    preamble: bool
    obeyed: bool | None = None


def score_translate(item: dict[str, Any], output: str) -> TranslateScore:
    from paraeval import langdetect  # local: langdetect imports metrics

    target = item["target_lang"]
    got = langdetect.detect(output)
    ok = got == target and not (target == "hi" and devanagari_broken(output))
    kept, total = protected_kept(item["text"], output, item.get("protected"))
    ref = item.get("reference", "")
    return TranslateScore(
        target_ok=ok,
        got_lang=got,
        kept=kept,
        protected=total,
        chrf=chrf(output, ref) if ref else 0.0,
        length_ratio=len(output) / max(len(ref), 1) if ref else 0.0,
        preamble=has_preamble(output, item["text"]),
        obeyed=obeyed_injection(item, output),
    )


@dataclass
class DraftScore:
    shape_ok: bool  # email: greeting and 2+ paragraphs; chat: one paragraph, no greeting line
    kept: int  # facts from the intent or the message the reply must carry
    protected: int
    words: int
    lang_ok: bool
    lang_reason: str
    preamble: bool
    obeyed: bool | None = None  # an injection, a scam, an invented detail or attachment claim


def score_draft(item: dict[str, Any], output: str) -> DraftScore:
    paragraphs = [p for p in re.split(r"\n\s*\n", output.strip()) if p.strip()]
    first = next((line for line in output.splitlines() if line.strip()), "")
    if item["preset"] == "email":
        shape = bool(_GREETING.match(first)) and len(paragraphs) >= 2
    else:
        shape = len(paragraphs) == 1 and len(output.split()) <= 80
    facts = list(item.get("protected") or [])
    ok, why = language_check(item["lang"], item["context"], output)
    return DraftScore(
        shape_ok=shape,
        kept=sum(f in output for f in facts),
        protected=len(facts),
        words=len(output.split()),
        lang_ok=ok,
        lang_reason=why,
        preamble=has_preamble(output, item["context"]),
        obeyed=obeyed_injection(item, output),
    )


@dataclass
class CustomScore:
    kept: int  # facts from the text the result must keep
    protected: int
    words: int
    lang_ok: bool
    lang_reason: str
    preamble: bool
    placeholder: bool
    obeyed: bool | None = None  # an instruction inside the text, or a forbidden result


def score_custom(item: dict[str, Any], output: str) -> CustomScore:
    facts = list(item.get("protected") or [])
    ok, why = language_check(item["lang"], item["text"], output)
    return CustomScore(
        kept=sum(f in output for f in facts),
        protected=len(facts),
        words=len(output.split()),
        lang_ok=ok,
        lang_reason=why,
        preamble=has_preamble(output, item["text"]),
        placeholder=bool(_PLACEHOLDER.search(output)) and not _PLACEHOLDER.search(item["text"]),
        obeyed=obeyed_injection(item, output),
    )


@dataclass
class ComposeScore:
    # email: Subject, greeting, the signer's name (or "[Your name]") last and no other
    # placeholder; message: no
    # greeting; post: at most 2 hashtags; note: bullets
    shape_ok: bool
    kept: int  # facts from the description the text must carry
    protected: int
    words: int
    lang_ok: bool
    lang_reason: str
    preamble: bool
    obeyed: bool | None = None  # invented details or an injection (the item's patterns)


def compose_shape(preset: str, output: str, signer: str = "") -> bool:
    """`signer` is the name the description signs with; without one the email ends with the
    mock's "[Your name]"."""
    lines = [line for line in output.splitlines() if line.strip()]
    if not lines:
        return False
    if preset == "email":
        return (
            lines[0].lower().startswith(("subject:", "विषय:"))
            and len(lines) > 1
            and bool(_GREETING.match(lines[1]))
            # main-window-compose.html: the sign-off is "[Your name]", the only placeholder,
            # unless the description gives the name to sign with
            and lines[-1].strip().lower() == (signer or "[your name]").lower()
            and not _PLACEHOLDER.search("\n".join(lines[:-1]))
        )
    if _PLACEHOLDER.search(output):
        return False  # only an email signs off, so nothing else has "[Your name]"
    if preset == "message":
        return not _GREETING.match(lines[0]) or len(lines[0].split()) > 4
    if preset == "post":
        return len(re.findall(r"#\w+", output)) <= 2
    if preset == "note":
        # A short title line saying what the note is about, then the items as bullets; never
        # a "Date:" line or markdown (owner, 2026-10-23).
        title = lines[0].strip()
        title_ok = (
            not title.startswith(("- ", "• ", "* ", "#", "**"))
            and len(title.split()) <= _NOTE_TITLE_WORDS
            and not _NOTE_LABEL.match(title)
        )
        bullets = sum(line.lstrip().startswith(("- ", "• ", "* ")) for line in lines[1:])
        return title_ok and bullets >= 2
    return True


def fact_kept(fact: str, output: str) -> bool:
    """Whether a fact survives, ignoring case and spaces: "7am" is kept as "7 am"."""
    squash = re.compile(r"\s+")
    return squash.sub("", fact.lower()) in squash.sub("", output.lower())


def score_compose(item: dict[str, Any], output: str) -> ComposeScore:
    facts = list(item.get("protected") or [])
    ok, why = language_check(item["lang"], item["text"], output)
    return ComposeScore(
        shape_ok=compose_shape(item["preset"], output, item.get("signer", "")),
        kept=sum(fact_kept(f, output) for f in facts),
        protected=len(facts),
        words=len(output.split()),
        lang_ok=ok,
        lang_reason=why,
        preamble=has_preamble(output, item["text"]),
        obeyed=obeyed_injection(item, output),
    )

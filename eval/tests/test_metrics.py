from paraeval import metrics


def test_tokenize_keeps_devanagari_signs_in_words() -> None:
    assert metrics.tokenize("मेरी बहन आई।") == ["मेरी", "बहन", "आई", "।"]
    assert metrics.tokenize("Hi, it's me!") == ["Hi", ",", "it's", "me", "!"]


def test_proofread_scores_edits_against_reference() -> None:
    item = {"text": "i has a apple", "reference": "I have an apple", "lang": "en"}
    s = metrics.score_proofread(item, "I have an apple")
    assert s.exact and s.fp == 0 and s.fn == 0 and s.tp > 0
    s = metrics.score_proofread(item, "I has an apple")
    assert not s.exact and s.fn > 0


def test_false_alarm_on_clean_text() -> None:
    item = {
        "text": "gonna grab lunch, want anything?",
        "reference": "gonna grab lunch, want anything?",
        "lang": "en",
    }
    assert not metrics.score_proofread(item, item["text"]).false_alarm
    assert metrics.score_proofread(item, "Going to grab lunch. Want anything?").false_alarm


def test_protected_tokens() -> None:
    src = "Pay $45.50 by 12/03 at https://pay.example.com or mail a@b.co, ref `x1` #bills"
    kept, total = metrics.protected_kept(src, src, ["Saoirse"])
    assert total == 7 and kept == 6  # Saoirse is not in the text
    kept, _ = metrics.protected_kept(src, src.replace("45.50", "45.5"))
    assert kept == 5


def test_language_floor() -> None:
    assert metrics.language_check("hi", "मैं कल आऊँगा", "मैं कल आऊँगा।")[0]
    assert not metrics.language_check("hi", "मैं कल आऊँगा", "I will come tomorrow.")[0]
    assert not metrics.language_check("hi", "x", "ि broken")[0]
    src = "bhai kal ka plan pakka hai na? 7 baje tak aa jaana"
    assert metrics.language_check("hinglish", src, "Haan bhai, pakka hai, 7 baje aa jaunga")[0]
    assert not metrics.language_check("hinglish", src, "Yes, the plan is confirmed for 7.")[0]
    assert not metrics.language_check("hinglish", src, "हाँ भाई, पक्का है")[0]


def test_preamble() -> None:
    assert metrics.has_preamble("Here is the corrected text:\nI have an apple.", "i has a apple")
    assert metrics.has_preamble('"I have an apple."', "i has a apple")
    assert not metrics.has_preamble("I have an apple.", "i has a apple")


def test_reply_scores() -> None:
    item = {"context": "Free at 5?", "lang": "en"}
    s = metrics.score_reply(
        item, '{"replies": ["Yes, see you then!", "Can we do 6?", "Sorry, busy."]}'
    )
    assert s.valid and s.distinct and s.lang_ok and s.count == 3
    assert not metrics.score_reply(item, "not json").valid
    s = metrics.score_reply(item, '{"replies": ["Yes!", "Yes!", "Sorry, busy."]}')
    assert not s.distinct

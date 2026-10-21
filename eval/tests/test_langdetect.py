from paraeval import langdetect

VECTORS = [
    ("Are you free tomorrow at 5?", "en"),
    ("you coming tonight?", "en"),
    ("मेरी बहन कल दिल्ली से आ रही है।", "hi"),
    ("Hi team, कल का deadline बहुत important है।", "hi"),
    ("bhai kal ka plan pakka hai na? 7 baje tak aa jaana", "hinglish"),
    ("Sir kal nahi aa paunga, tabiyat thik nahi hai", "hinglish"),
    ("", "en"),
]


def test_detect_vectors() -> None:
    for text, lang in VECTORS:
        assert langdetect.detect(text) == lang, text


def test_inputs_name_the_language() -> None:
    assert langdetect.inputs_for("you coming tonight?")["lang_name"] == "English"

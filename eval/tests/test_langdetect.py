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


OTHER_LANGUAGES = [
    ("Bonjour, est-ce que tu viens ce soir ? Je t'attends au café.", "fr"),
    ("Hola, ¿vienes esta noche? Te espero en el café.", "es"),
    ("Hallo, kommst du heute Abend? Ich warte im Café.", "de"),
    ("Olá, você vem hoje à noite? Estou te esperando.", "pt"),
    ("Halo, kamu datang nanti malam? Saya tunggu di kafe.", "id"),
    ("நீங்கள் இன்று மாலை வருகிறீர்களா?", "ta"),
    ("আজ সন্ধ্যায় তুমি আসবে?", "bn"),
    ("मी आज रात्री येऊ का? मी वाट पाहतो.", "mr"),
    ("तपाईं आज बेलुका आउनुहुन्छ? म पर्खिरहेको छु।", "ne"),
    ("کیا آپ آج رات آئیں گے؟", "ur"),
    ("هل ستأتي الليلة؟", "ar"),
    ("你今晚来吗？", "zh"),
    ("今夜来ますか？", "ja"),
    ("Ти прийдеш сьогодні ввечері?", "uk"),
    ("Ты придёшь сегодня вечером?", "und"),  # Cyrillic without markers: never guess Russian
]


def test_other_languages_are_never_called_english_or_hindi() -> None:
    for text, lang in OTHER_LANGUAGES:
        assert langdetect.detect(text) == lang, text


def test_unknown_language_prompt_names_no_language() -> None:
    name = langdetect.inputs_for("Ты придёшь сегодня вечером?")["lang_name"]
    assert "English" not in name and "Hindi" not in name
    assert langdetect.inputs_for("Bonjour, est-ce que tu viens ce soir ?")["lang_name"] == "French"


def test_other_language_check_fails_only_on_clear_evidence() -> None:
    from paraeval.metrics import language_check

    assert language_check("fr", "Bonjour, tu viens ce soir ?", "Oui, je viens ce soir !")[0]
    assert not language_check("fr", "Bonjour, tu viens ce soir ?", "Yes, I will be there tonight.")[
        0
    ]
    assert not language_check("ta", "நீங்கள் வருகிறீர்களா?", "Yes, I am coming.")[0]
    assert language_check("es", "Hola, ¿vienes esta noche?", "Sí!")[0]  # too short to judge
    assert language_check("und", "Ты придёшь сегодня вечером?", "Да, приду.")[0]

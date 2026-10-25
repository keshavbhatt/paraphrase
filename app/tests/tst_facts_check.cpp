// The facts a rewrite must keep (popup-rewrite.html, note 4): numbers, dates, money, links,
// mentions and names are found in the original and checked in the result.

#include "ui/core/facts_check.h"

#include <QTest>

using namespace Qt::StringLiterals;
using paraphrase::app::checkFacts;
using paraphrase::app::factsLine;

class TestFactsCheck : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void keepsNamesNumbersAndDates()
    {
        const auto check =
            checkFacts(u"yo Sam, need Friday off. paid $8,500 on March 19, see https://x.io/a."_s,
                       u"Sam, I need Friday off. I paid $8,500 on March 19 (https://x.io/a)."_s);
        QVERIFY(check.missing.isEmpty());
        QVERIFY(check.kept.contains(u"Sam"_s));
        QVERIFY(check.kept.contains(u"Friday"_s));
        QVERIFY(check.kept.contains(u"8,500"_s));
        QVERIFY(check.kept.contains(u"https://x.io/a"_s));
        QVERIFY(factsLine(check).startsWith(u"kept: "_s));
    }

    void saysWhatIsMissing()
    {
        const auto check =
            checkFacts(u"Meet Priya on Friday at 3 pm."_s, u"Let us meet on Thursday at 3 pm."_s);
        QCOMPARE(check.missing, (QStringList{u"Priya"_s, u"Friday"_s}));
        QCOMPARE(factsLine(check), u"check: Priya, Friday are missing"_s);
    }

    void checksOnlyNumbersAcrossLanguages()
    {
        // The user chose Hindi: "Friday" and "Thai" are written in Devanagari, so they are not
        // looked for (owner, 2026-10-25: "check: Friday, Thai are missing"); numbers still are.
        const auto check = checkFacts(u"Dinner on Friday at 8 at the Thai place, table 12?"_s,
                                      u"शुक्रवार को 8 बजे थाई जगह पर डिनर, टेबल 12?"_s, false);
        QVERIFY(check.missing.isEmpty());
        QCOMPARE(check.kept, (QStringList{u"8"_s, u"12"_s}));
        const auto lost = checkFacts(u"Order 7781 arrives in 5 days."_s, u"ऑर्डर 5 दिन में आएगा।"_s, false);
        QCOMPARE(lost.missing, QStringList{u"7781"_s});
    }

    void ignoresSentenceStartsAndShortTexts()
    {
        QVERIFY(checkFacts(u"Thanks. Please send it."_s, u"Please send it, thanks."_s).isEmpty());
        QCOMPARE(factsLine(checkFacts(u"ok"_s, u"OK"_s)), QString());
    }
};

QTEST_GUILESS_MAIN(TestFactsCheck)
#include "tst_facts_check.moc"

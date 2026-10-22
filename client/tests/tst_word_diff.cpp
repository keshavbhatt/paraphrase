// The word diff clients show for proofread (PLAN D6): changes are whole words or marks, one
// per correction, each can be undone on its own, and no script is split mid-letter.

#include "word_diff.h"

#include <QTest>

using namespace Qt::StringLiterals;
using paraphrase::client::DiffSegment;
using paraphrase::client::WordDiff;

namespace {

/// The changes as "before>after" pairs.
QStringList pairs(const WordDiff& diff)
{
    QStringList out;
    for (const qsizetype k : diff.changes()) {
        out.append(diff.segments().at(k).before + u'>' + diff.segments().at(k).after);
    }
    return out;
}

} // namespace

class TestWordDiff : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void tokensGiveTheTextBack()
    {
        const QString text = u"Hello,  world!\nIt's 5pm: मेरी बहन आ रही है।"_s;
        QCOMPARE(WordDiff::tokens(text).join(QString()), text);
        QCOMPARE(WordDiff::tokens(u"It's 5pm."_s), (QStringList{u"It's"_s, u" "_s, u"5pm"_s, u"."_s}));
    }

    void findsEachCorrection()
    {
        // The proofread mock's recipe line.
        const WordDiff diff(u"Whisk the egg's with the milk and add it slowly to the dry ingredients"_s,
                            u"Whisk the eggs with the milk and add it slowly to the dry ingredients."_s);
        QCOMPARE(pairs(diff), (QStringList{u"egg's>eggs"_s, u">."_s}));
        QCOMPARE(diff.result(), u"Whisk the eggs with the milk and add it slowly to the dry ingredients."_s);
    }

    void undoesOneChangeAtATime()
    {
        const QString before = u"Let the batter rest for 10 minute. Bubbles appears."_s;
        const QString after = u"Let the batter rest for 10 minutes. Bubbles appear."_s;
        const WordDiff diff(before, after);
        const QList<qsizetype> changes = diff.changes();
        QCOMPARE(changes.size(), 2);
        QCOMPARE(diff.result({changes.at(0)}), u"Let the batter rest for 10 minute. Bubbles appear."_s);
        QCOMPARE(diff.result({changes.at(1)}), u"Let the batter rest for 10 minutes. Bubbles appears."_s);
        QCOMPARE(diff.result({changes.at(0), changes.at(1)}), before);
    }

    void keepsDevanagariLettersWhole()
    {
        const WordDiff diff(u"मेरा बहन कल आ रहा है"_s, u"मेरी बहन कल आ रही है।"_s);
        QCOMPARE(pairs(diff), (QStringList{u"मेरा>मेरी"_s, u"रहा>रही"_s, u">।"_s}));
    }

    void reportsNoChangeForTheSameText()
    {
        const WordDiff diff(u"Nothing to fix here."_s, u"Nothing to fix here."_s);
        QVERIFY(diff.changes().isEmpty());
        QCOMPARE(diff.segments(), (QList<DiffSegment>{{.before = u"Nothing to fix here."_s,
                                                       .after = u"Nothing to fix here."_s,
                                                       .changed = false}}));
    }

    void handlesEmptyTexts()
    {
        QCOMPARE(WordDiff(QString(), u"Hi."_s).result(), u"Hi."_s);
        QCOMPARE(WordDiff(u"Hi."_s, QString()).result(), QString());
        QVERIFY(WordDiff(QString(), QString()).segments().isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestWordDiff)
#include "tst_word_diff.moc"

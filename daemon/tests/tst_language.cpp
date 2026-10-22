// The C++ language logic gives the Python reference's answer for every shared vector
// (tests/vectors/language.json, written by eval/export_vectors.py).

#include "util/language_detector.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

using namespace Qt::StringLiterals;
using paraphrase::util::LanguageData;
using paraphrase::util::LanguageDetector;

namespace {

QJsonObject vectors()
{
    QFile file(QStringLiteral(PARAPHRASE_REPO_ROOT "/tests/vectors/language.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}

QString shortText(const QString& text)
{
    return text.left(60).replace(u'\n', u' ');
}

} // namespace

class TestLanguage : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void builtInDataLoads()
    {
        const LanguageData data = LanguageData::builtIn();
        QVERIFY(data.hinglishWords.contains(u"hai"_s));
        QCOMPARE(data.latinStopwords.first().first, u"en"_s);
        QCOMPARE(data.names.value(u"ta"_s), u"Tamil"_s);
        QVERIFY(!data.unknownPromptName.isEmpty());
    }

    void detectMatchesPython()
    {
        const QJsonObject v = vectors();
        const QJsonArray cases = v.value(u"detect"_s).toArray();
        QVERIFY2(cases.size() > 500, "tests/vectors/language.json missing or short");
        const LanguageDetector detector(LanguageData::builtIn());
        int wrong = 0;
        for (const auto& c : cases) {
            const QJsonObject o = c.toObject();
            const QString text = o.value(u"text"_s).toString();
            const QString lang = detector.detect(text);
            const bool same = lang == o.value(u"lang"_s).toString() &&
                              detector.promptName(lang) == o.value(u"prompt_name"_s).toString() &&
                              detector.plainName(lang) == o.value(u"plain_name"_s).toString();
            if (!same) {
                ++wrong;
                qWarning().noquote() << "detect" << shortText(text) << "C++:" << lang
                                     << "Python:" << o.value(u"lang"_s).toString();
            }
        }
        QCOMPARE(wrong, 0);
    }

    void checkMatchesPython()
    {
        const QJsonArray cases = vectors().value(u"language_check"_s).toArray();
        QVERIFY2(cases.size() > 300, "tests/vectors/language.json missing or short");
        const LanguageDetector detector(LanguageData::builtIn());
        int wrong = 0;
        for (const auto& c : cases) {
            const QJsonObject o = c.toObject();
            const auto result = detector.check(o.value(u"lang"_s).toString(), o.value(u"source"_s).toString(),
                                               o.value(u"output"_s).toString());
            if (result.ok != o.value(u"ok"_s).toBool() || result.reason != o.value(u"reason"_s).toString()) {
                ++wrong;
                qWarning().noquote() << "check" << o.value(u"lang"_s).toString()
                                     << shortText(o.value(u"output"_s).toString()) << "C++:" << result.ok
                                     << result.reason << "Python:" << o.value(u"ok"_s).toBool()
                                     << o.value(u"reason"_s).toString();
            }
        }
        QCOMPARE(wrong, 0);
    }
};

QTEST_GUILESS_MAIN(TestLanguage)
#include "tst_language.moc"

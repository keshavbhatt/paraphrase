#include "template_renderer.h"

#include <QRegularExpression>

#include <functional>

using namespace Qt::StringLiterals;

namespace paraphrase::skills {
namespace {

const QRegularExpression& varPattern()
{
    static const QRegularExpression re(uR"(\{\{\s*([a-z_][a-z0-9_]*)\s*\}\})"_s);
    return re;
}

const QRegularExpression& ifPattern()
{
    static const QRegularExpression re(uR"(\{\{#if\s+([a-z_][a-z0-9_]*)\s*\}\}(.*?)\{\{/if\}\})"_s,
                                       QRegularExpression::DotMatchesEverythingOption);
    return re;
}

const QRegularExpression& unlessPattern()
{
    static const QRegularExpression re(uR"(\{\{#unless\s+([a-z_][a-z0-9_]*)\s*\}\}(.*?)\{\{/unless\}\})"_s,
                                       QRegularExpression::DotMatchesEverythingOption);
    return re;
}

/// Python's re.sub: replaces every non-overlapping match, left to right.
QString substitute(const QString& text, const QRegularExpression& re,
                   const std::function<QString(const QRegularExpressionMatch&)>& replace)
{
    QString out;
    qsizetype last = 0;
    auto it = re.globalMatch(text);
    while (it.hasNext()) {
        const auto m = it.next();
        out += QStringView(text).sliced(last, m.capturedStart(0) - last);
        out += replace(m);
        last = m.capturedEnd(0);
    }
    out += QStringView(text).sliced(last);
    return out;
}

bool filled(const Values& values, const QString& name)
{
    return !values.value(name).trimmed().isEmpty();
}

} // namespace

QString renderTemplate(const QString& templ, const Values& values)
{
    const auto keepIf = [&values](const QRegularExpressionMatch& m) {
        return filled(values, m.captured(1)) ? m.captured(2) : QString();
    };
    const auto keepUnless = [&values](const QRegularExpressionMatch& m) {
        return filled(values, m.captured(1)) ? QString() : m.captured(2);
    };
    QString out = templ;
    QString prev;
    do { // nested blocks resolve from the inside out
        prev = out;
        out = substitute(substitute(out, ifPattern(), keepIf), unlessPattern(), keepUnless);
    } while (out != prev);
    return substitute(out, varPattern(),
                      [&values](const QRegularExpressionMatch& m) { return values.value(m.captured(1)); });
}

QSet<QString> templateVariables(const QString& templ)
{
    QSet<QString> out;
    for (const auto* re : {&varPattern(), &ifPattern(), &unlessPattern()}) {
        auto it = re->globalMatch(templ);
        while (it.hasNext()) {
            out.insert(it.next().captured(1));
        }
    }
    return out;
}

QSet<QString> delimiterTags(const QString& templ)
{
    static const QRegularExpression tag(u"</?([a-z_]+)>"_s);
    QSet<QString> out;
    auto it = tag.globalMatch(templ);
    while (it.hasNext()) {
        out.insert(it.next().captured(1));
    }
    return out;
}

QString neutralise(QString value, const QSet<QString>& tags)
{
    for (const QString& tag : tags) {
        const QRegularExpression fence(u"<(/?)(%1)>"_s.arg(tag), QRegularExpression::CaseInsensitiveOption);
        value = substitute(value, fence, [](const QRegularExpressionMatch& m) {
            return u"< "_s + m.captured(1) + m.captured(2) + u'>';
        });
    }
    return value;
}

} // namespace paraphrase::skills

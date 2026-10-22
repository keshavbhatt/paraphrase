#include "languages_section.h"

#include "widgets.h"
#include "window_style.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

QFrame* testedBox(const QString& name)
{
    auto* box = new QFrame;
    box->setObjectName(u"lang"_s);
    auto* col = new QVBoxLayout(box);
    col->setContentsMargins(14, 11, 14, 11);
    col->setSpacing(2);
    QLabel* title = makeLabel(name);
    title->setStyleSheet(u"font-weight: 600;"_s);
    col->addWidget(title);
    col->addWidget(makeLabel(LanguagesSection::tr("Tested · all actions"), u"success"_s));
    return box;
}

} // namespace

LanguagesSection::LanguagesSection(QWidget* parent)
    : QFrame(parent)
{
    setObjectName(u"card"_s);
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(18, 16, 18, 16);
    col->setSpacing(10);
    col->addWidget(makeLabel(tr("Languages"), u"h2"_s));
    QLabel* about =
        makeLabel(tr("Paraphrase answers in the language you wrote in. Tested languages were checked by "
                     "the Paraphrase team with these models; others work as well as the model allows."),
                  u"small"_s);
    about->setWordWrap(true);
    about->setMaximumWidth(580);
    col->addWidget(about);
    auto* row = new QHBoxLayout;
    row->setSpacing(10);
    for (const QString& name : {tr("English"), tr("Hindi"), tr("Hinglish")}) {
        row->addWidget(testedBox(name), 1);
    }
    col->addLayout(row);
    QLabel* rest =
        makeLabel(tr("Best effort: Marathi, Bengali, Tamil, Spanish, French, German, Portuguese, Chinese "
                     "and many more."));
    rest->setStyleSheet(u"font-size: 12px;"_s);
    rest->setWordWrap(true);
    col->addWidget(rest);
}

} // namespace paraphrase::app

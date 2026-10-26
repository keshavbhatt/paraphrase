#include "ui/settings/languages_section.h"

#include "ui/common/widgets.h"
#include "ui/common/window_style.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kGap = 12;
constexpr int kUnderHeading = 4;

QFrame* testedBox(const QString& name)
{
    auto* box = new QFrame;
    box->setObjectName(u"lang"_s);
    auto* col = new QVBoxLayout(box);
    col->setContentsMargins(14, 11, 14, 11);
    col->setSpacing(2);
    // A style role, not a widget style sheet: that one measured the label in another font, and
    // the descenders were cut ("Enalish").
    col->addWidget(makeLabel(name, u"strong"_s));
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
    col->setSpacing(kGap); // settings-models.html: 12 around the boxes
    auto* head = new QVBoxLayout;
    head->setSpacing(kUnderHeading); // the line sits 4 under the heading
    head->addWidget(makeLabel(tr("Languages"), u"h2"_s));
    QLabel* about =
        makeLabel(tr("Paraphrase answers in the language you wrote in. Tested languages were checked by "
                     "the Paraphrase team with these models; others work as well as the model allows."),
                  u"small"_s);
    about->setWordWrap(true);
    about->setMaximumWidth(580);
    head->addWidget(about);
    col->addLayout(head);
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

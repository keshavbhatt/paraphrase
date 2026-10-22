#include "shortcut_page.h"

#include "widgets.h"

#include <QLabel>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

ShortcutPage::ShortcutPage(QWidget* parent)
    : QWidget(parent)
{
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(24, 22, 24, 22);
    col->setSpacing(12);
    col->addWidget(makeLabel(tr("A shortcut that opens Paraphrase over any app"), u"h1"_s));
    QLabel* text =
        makeLabel(tr("Select text anywhere, press the shortcut, pick an action. This comes in a later "
                     "version of Paraphrase. Until then everything works in Paraphrase's own window: "
                     "paste your text, pick an action, copy the result."),
                  u"muted"_s);
    text->setWordWrap(true);
    text->setMaximumWidth(620);
    col->addWidget(text);
    col->addStretch();
}

} // namespace paraphrase::app

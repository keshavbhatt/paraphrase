#include "text_page.h"

#include "widgets.h"
#include "window_style.h"

#include <QLabel>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

TextPage::TextPage(const QString& title, const QStringList& paragraphs, QWidget* parent)
    : QWidget(parent)
    , m_column(new QVBoxLayout(this))
{
    m_column->setContentsMargins(0, 0, 0, 0);
    m_column->setSpacing(10);
    m_column->addWidget(makeLabel(title, u"h1"_s));
    for (qsizetype i = 0; i < paragraphs.size(); ++i) {
        QLabel* p = makeLabel(paragraphs.at(i), i == 0 ? QString() : u"muted"_s);
        p->setWordWrap(true);
        p->setMaximumWidth(560);
        p->setTextFormat(Qt::RichText);
        p->setOpenExternalLinks(true);
        m_column->addWidget(p);
    }
    m_column->addStretch();
}

} // namespace paraphrase::app

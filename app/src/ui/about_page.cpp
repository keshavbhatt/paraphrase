#include "about_page.h"

#include "models_controller.h"
#include "window_style.h"

#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

QLabel* paragraph(const QString& text, const QString& role = {})
{
    QLabel* l = textLabel(text, role);
    l->setWordWrap(true);
    l->setMaximumWidth(560);
    return l;
}

} // namespace

AboutPage::AboutPage(ModelsController& models, QWidget* parent)
    : QWidget(parent)
    , m_models(models)
    , m_modelLines(new QVBoxLayout)
{
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(10);
    col->addWidget(textLabel(tr("About Paraphrase"), u"h1"_s));
    QLabel* version = textLabel(tr("Version %1").arg(QApplication::applicationVersion()), u"muted"_s);
    version->setObjectName(u"version"_s);
    version->setTextInteractionFlags(Qt::TextSelectableByMouse);
    col->addWidget(version);
    col->addWidget(paragraph(tr("A private writing assistant: proofread, change tone and reply, with a language model "
                                "that runs on this computer. Free software under the GNU General Public License, "
                                "version 3 or later.")));
    col->addSpacing(6);
    col->addWidget(textLabel(tr("Built with"), u"h2"_s));
    col->addWidget(paragraph(tr("llama.cpp (MIT), yaml-cpp (MIT), Qt (LGPL-3.0), Lucide icons (ISC)."), u"muted"_s));
    col->addSpacing(6);
    col->addWidget(textLabel(tr("Models"), u"h2"_s));
    m_modelLines->setSpacing(4);
    col->addLayout(m_modelLines);
    col->addStretch();
    connect(&m_models, &ModelsController::changed, this, &AboutPage::reload);
}

void AboutPage::setTheme(const Tokens& tokens)
{
    m_tokens = tokens;
    reload();
}

void AboutPage::reload()
{
    while (QLayoutItem* item = m_modelLines->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    for (const ModelInfo& m : m_models.models()) {
        if (!m.isModel() || !m.installed) {
            continue;
        }
        QLabel* line = textLabel(tr("%1 by %2, %3")
                                     .arg(m.name.toHtmlEscaped(), m.maker().toHtmlEscaped(),
                                          link(m.licenceUrl, m.licence.toHtmlEscaped(), m_tokens)),
                                 u"muted"_s);
        line->setTextFormat(Qt::RichText);
        line->setOpenExternalLinks(true);
        m_modelLines->addWidget(line);
    }
    if (m_modelLines->isEmpty()) {
        m_modelLines->addWidget(textLabel(tr("No model is installed yet."), u"muted"_s));
    }
}

} // namespace paraphrase::app

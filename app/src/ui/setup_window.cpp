#include "setup_window.h"

#include "chip_button.h"
#include "model_page.h"
#include "models_controller.h"
#include "sample_run.h"
#include "shortcut_page.h"
#include "step_header.h"
#include "try_page.h"
#include "widgets.h"
#include "window_style.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

SetupWindow::SetupWindow(client::DaemonClient& client, QWidget* parent)
    : QWidget(parent)
    , m_tokens(Theme::current())
    , m_models(new ModelsController(client, this))
    , m_run(new SampleRun(client, this))
    , m_header(new StepHeader(tr("Set up Paraphrase"), {tr("Shortcut (later)"), tr("Model"), tr("Try it")}))
    , m_pages(new QStackedWidget)
    , m_modelPage(new ModelPage(*m_models))
{
    setObjectName(u"setupWindow"_s);
    setWindowTitle(tr("Set up Paraphrase"));
    resize(820, 700);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(m_header);
    m_pages->addWidget(new ShortcutPage);
    m_pages->addWidget(m_modelPage);
    m_pages->addWidget(new TryPage(*m_run));
    root->addWidget(m_pages, 1);
    root->addWidget(buildFooter());
    connect(m_modelPage, &ModelPage::readyChanged, this, &SetupWindow::updateFooter);
    applyTheme();
    showStep(Model);
}

SetupWindow::~SetupWindow() = default;

SetupWindow::Step SetupWindow::step() const
{
    return static_cast<Step>(m_pages->currentIndex());
}

void SetupWindow::start()
{
    m_models->refresh();
    m_modelPage->load();
}

QWidget* SetupWindow::buildFooter()
{
    auto* band = new QFrame;
    band->setObjectName(u"footBand"_s);
    auto* row = new QHBoxLayout(band);
    row->setContentsMargins(24, 14, 24, 14);
    row->setSpacing(10);
    m_back = makeButton(tr("Back"));
    m_back->setProperty("size", u"big"_s);
    row->addWidget(m_back);
    row->addStretch();
    m_footNote = makeLabel(tr("You can change models later in Settings"), u"small"_s);
    row->addWidget(m_footNote);
    m_next = new ChipButton(tr("Continue when ready"));
    m_next->setObjectName(u"next"_s);
    m_next->setProperty("kind", u"primary"_s);
    m_next->setProperty("size", u"big"_s);
    row->addWidget(m_next);
    connect(m_back, &QPushButton::clicked, this, [this] { showStep(step() - 1); });
    connect(m_next, &QPushButton::clicked, this, [this] {
        if (step() == Try) {
            Q_EMIT finished();
        } else {
            showStep(step() + 1);
        }
    });
    return band;
}

void SetupWindow::showStep(int step)
{
    m_pages->setCurrentIndex(std::clamp(step, 0, 2));
    // The shortcut is not built yet, so it is never shown as done (the mock ticks it).
    QList<int> done;
    if (this->step() == Try) {
        done.append(Model);
    }
    m_header->setStep(this->step(), done);
    m_back->setVisible(this->step() != Shortcut);
    m_footNote->setVisible(this->step() == Model);
    updateFooter();
}

void SetupWindow::updateFooter()
{
    switch (step()) {
    case Model: {
        // Continue once a ticked model is installed; the others keep downloading.
        const bool ready = m_modelPage->ready();
        m_next->setLabel(ready ? tr("Continue") : tr("Continue when ready"));
        m_next->setEnabled(ready);
        break;
    }
    case Try:
        m_next->setLabel(tr("Open Paraphrase"));
        m_next->setEnabled(true);
        break;
    case Shortcut:
        m_next->setLabel(tr("Next"));
        m_next->setEnabled(true);
        break;
    }
}

void SetupWindow::applyTheme()
{
    m_tokens = Theme::current();
    setStyleSheet(windowStyleSheet(m_tokens));
    m_modelPage->setLockIcon(Theme::icon(u"lock"_s, m_tokens.privacy, 14).pixmap(14, 14));
}

void SetupWindow::changeEvent(QEvent* event)
{
    if ((event->type() == QEvent::ThemeChange || event->type() == QEvent::PaletteChange) &&
        Theme::current().dark != m_tokens.dark) {
        applyTheme();
    }
    QWidget::changeEvent(event);
}

} // namespace paraphrase::app

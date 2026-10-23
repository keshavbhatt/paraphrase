#include "ui/setup/setup_window.h"

#include "ui/common/chip_button.h"
#include "ui/common/widgets.h"
#include "ui/common/window_style.h"
#include "ui/models/models_controller.h"
#include "ui/setup/model_page.h"
#include "ui/setup/sample_run.h"
#include "ui/setup/shortcut_page.h"
#include "ui/setup/step_header.h"
#include "ui/setup/try_page.h"

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
    , m_header(new StepHeader(tr("Set up Paraphrase"), {tr("Over any app"), tr("Model"), tr("Try it")}))
    , m_pages(new QStackedWidget)
    , m_modelPage(new ModelPage(*m_models))
    , m_desktopPage(new ShortcutPage)
{
    setObjectName(u"setupWindow"_s);
    setWindowTitle(tr("Set up Paraphrase"));
    resize(820, 700);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(m_header);
    m_pages->addWidget(m_desktopPage);
    m_pages->addWidget(m_modelPage);
    m_pages->addWidget(new TryPage(*m_run));
    root->addWidget(m_pages, 1);
    root->addWidget(buildFooter());
    connect(m_modelPage, &ModelPage::readyChanged, this, &SetupWindow::updateFooter);
    applyTheme();
    showStep(Shortcut); // setup-desktop.html: over any app first, both optional
}

SetupWindow::~SetupWindow() = default;

void SetupWindow::setDesktopSupport(const DesktopSupport& support)
{
    m_desktopPage->setSupport(support);
}

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
    m_back->setProperty("btnSize", u"big"_s);
    row->addWidget(m_back);
    m_skip = makeButton(tr("Skip for now"), u"flat"_s);
    m_skip->setObjectName(u"skip"_s);
    m_skip->setProperty("btnSize", u"big"_s);
    row->addWidget(m_skip);
    connect(m_skip, &QPushButton::clicked, this, [this] { showStep(Model); });
    row->addStretch();
    m_footNote = makeLabel(tr("You can change models later in Settings"), u"small"_s);
    row->addWidget(m_footNote);
    m_next = new ChipButton(tr("Continue when ready"));
    m_next->setObjectName(u"next"_s);
    m_next->setProperty("kind", u"primary"_s);
    m_next->setProperty("btnSize", u"big"_s);
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
    QList<int> done;
    if (this->step() != Shortcut) {
        done.append(Shortcut);
    }
    if (this->step() == Try) {
        done.append(Model);
    }
    m_header->setStep(this->step(), done);
    m_back->setVisible(this->step() != Shortcut);
    m_skip->setVisible(this->step() == Shortcut);
    m_footNote->setVisible(this->step() != Try);
    m_footNote->setText(this->step() == Shortcut ? tr("You can change both later in Settings")
                                                 : tr("You can change models later in Settings"));
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
    // setup-desktop.html: "Next" with its arrow; the other steps' buttons have none.
    m_next->setTrailingIcon(step() == Shortcut ? Theme::icon(u"forward"_s, QColor(Qt::white), 14) : QIcon());
}

void SetupWindow::applyTheme()
{
    m_tokens = Theme::current();
    setStyleSheet(windowStyleSheet(m_tokens));
    m_modelPage->setLockIcon(Theme::icon(u"lock"_s, m_tokens.privacy, 14).pixmap(14, 14));
    m_desktopPage->applyTheme(m_tokens);
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

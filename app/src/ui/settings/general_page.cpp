#include "ui/settings/general_page.h"

#include "ui/common/toggle_switch.h"
#include "ui/common/widgets.h"
#include "ui/core/preferences.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include <algorithm>
#include <array>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr std::array<QStringView, 3> kAppearances = {u"system", u"light", u"dark"};

} // namespace

GeneralPage::GeneralPage(Preferences& preferences, bool hasTray, QWidget* parent)
    : QWidget(parent)
    , m_preferences(preferences)
    , m_login(new ToggleSwitch)
    , m_tray(new ToggleSwitch)
    , m_appearance(new SegmentedControl({
          {.name = u"appearanceSystem"_s, .text = tr("System")},
          {.name = u"appearanceLight"_s, .text = tr("Light")},
          {.name = u"appearanceDark"_s, .text = tr("Dark")},
      }))
{
    m_login->setObjectName(u"startAtLogin"_s);
    m_tray->setObjectName(u"keepInTray"_s);
    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(18);
    column->addWidget(makeLabel(tr("General"), u"h1"_s));
    auto* card = new QFrame;
    card->setObjectName(u"card"_s);
    auto* options = new QVBoxLayout(card);
    options->setContentsMargins(18, 6, 18, 6);
    options->setSpacing(0);
    options->addWidget(
        option(tr("Start Paraphrase when I log in"),
               tr("It waits in the tray, so the shortcut works at once. It loads a model only when "
                  "you use it."),
               m_login));
    if (hasTray) {
        options->addWidget(option(tr("Keep running in the tray when I close the window"),
                                  tr("The shortcut keeps working. Quit from the tray icon."), m_tray));
    } else {
        m_tray->setParent(this); // without a tray there is nothing to keep running in
        m_tray->hide();
    }
    options->addWidget(option(tr("Appearance"), tr("Follows your desktop unless you choose."), m_appearance));
    column->addWidget(card);
    column->addStretch();

    connect(m_login, &QAbstractButton::toggled, &m_preferences, &Preferences::setStartAtLogin);
    connect(m_tray, &QAbstractButton::toggled, &m_preferences, &Preferences::setKeepInTray);
    connect(m_appearance, &SegmentedControl::changed, this, [this](int index) {
        const auto i =
            static_cast<std::size_t>(std::clamp(index, 0, static_cast<int>(kAppearances.size()) - 1));
        m_preferences.setAppearance(kAppearances.at(i).toString());
    });
    reload();
}

QWidget* GeneralPage::option(const QString& title, const QString& detail, QWidget* control)
{
    auto* row = new QFrame;
    row->setObjectName(u"actionRow"_s); // the same divided rows as My actions
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 12, 0, 12);
    layout->setSpacing(12);
    auto* what = new QVBoxLayout;
    what->setSpacing(2);
    what->addWidget(makeLabel(u"<b>%1</b>"_s.arg(title.toHtmlEscaped())));
    QLabel* text = makeLabel(detail, u"small"_s);
    text->setWordWrap(true);
    what->addWidget(text);
    layout->addLayout(what, 1);
    layout->addWidget(control, 0, Qt::AlignTop);
    return row;
}

void GeneralPage::reload()
{
    const QSignalBlocker loginBlock(m_login);
    const QSignalBlocker trayBlock(m_tray);
    m_login->setChecked(Preferences::startAtLogin());
    m_tray->setChecked(Preferences::keepInTray());
    const auto* const it = std::ranges::find(kAppearances, Preferences::appearance());
    m_appearance->setCurrent(it == kAppearances.end() ? 0 : static_cast<int>(it - kAppearances.begin()));
}

void GeneralPage::applyTheme(const Tokens& t)
{
    m_login->applyTheme(t);
    m_tray->applyTheme(t);
}

} // namespace paraphrase::app

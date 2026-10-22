#include "action_bar.h"

#include "chip_button.h"
#include "flow_layout.h"
#include "languages.h"
#include "util/language_detector.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QMenu>
#include <QShortcut>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kMaxShortcuts = 9;
constexpr int kArrowSize = 14;

} // namespace

ActionBar::ActionBar(const util::LanguageDetector& detector, Keys keys, QWidget* parent)
    : QWidget(parent)
    , m_detector(detector)
    , m_keys(keys)
{
    if (keys == Keys::Digits) {
        m_row = new FlowLayout(this);
    } else {
        auto* row = new QHBoxLayout(this);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        m_row = row;
    }
}

void ActionBar::setCatalog(const SkillCatalog* catalog)
{
    m_catalog = catalog;
    auto* group = new QButtonGroup(this);
    group->setExclusive(true);
    int number = 0;
    for (const QString& base : catalog->actionBases()) {
        ChipButton* b = addAction(++number, base);
        group->addButton(b);
        m_row->addWidget(b);
        m_buttons.insert(base, b);
    }
    if (auto* row = qobject_cast<QHBoxLayout*>(m_row)) {
        row->addStretch();
    }
    updateArrows();
}

ChipButton* ActionBar::addAction(int number, const QString& base)
{
    const bool digits = m_keys == Keys::Digits;
    QString chip;
    if (number <= kMaxShortcuts) {
        chip = digits ? QString::number(number) : tr("Ctrl %1").arg(number);
    }
    auto* b = new ChipButton(m_catalog->actionTitle(base), chip);
    b->setProperty("kind", u"action"_s);
    b->setObjectName(u"action-"_s + base);
    b->setCheckable(true);
    const auto variants = m_catalog->variants(base);
    if (base == u"reply"_s) {
        connect(b, &QPushButton::clicked, this, &ActionBar::replyChosen);
    } else if (m_catalog->picksLanguage(base)) {
        b->setMenu(languageMenu(b, variants.front()->id));
    } else if (variants.size() > 1) {
        b->setMenu(presetMenu(b, base));
    } else {
        const QString id = variants.front()->id;
        connect(b, &QPushButton::clicked, this, [this, id] { Q_EMIT chosen(id, {}); });
    }
    if (number <= kMaxShortcuts) {
        const QKeySequence keys(digits ? QString::number(number) : u"Ctrl+%1"_s.arg(number));
        const auto* shortcut = new QShortcut(keys, window());
        connect(shortcut, &QShortcut::activated, b, [b] {
            if (b->menu() != nullptr) {
                b->showMenu();
            } else {
                b->click();
            }
        });
    }
    return b;
}

QMenu* ActionBar::presetMenu(ChipButton* button, const QString& base)
{
    auto* menu = new QMenu(button);
    for (const SkillInfo* v : m_catalog->variants(base)) {
        QAction* item = menu->addAction(SkillCatalog::presetTitle(*v));
        item->setObjectName(v->id);
        const QString id = v->id;
        connect(item, &QAction::triggered, this, [this, id] { Q_EMIT chosen(id, {}); });
    }
    return menu;
}

QMenu* ActionBar::languageMenu(ChipButton* button, const QString& skillId)
{
    // Filled when it opens, so the text's own language is left out.
    auto* menu = new QMenu(button);
    connect(menu, &QMenu::aboutToShow, this, [this, menu, skillId] { fillLanguages(menu, skillId); });
    return menu;
}

void ActionBar::fillLanguages(QMenu* menu, const QString& skillId)
{
    menu->clear();
    const QString own = m_textLanguage ? m_textLanguage() : QString();
    bool bestEffort = false;
    for (const MenuLanguage& lang : menuLanguages()) {
        const QString name = languageName(m_detector, lang.code);
        if (name.isEmpty() || lang.code == own) {
            continue;
        }
        if (!lang.tested && !bestEffort) {
            menu->addSection(tr("Best effort"));
            bestEffort = true;
        }
        QAction* item = menu->addAction(name);
        item->setObjectName(u"lang-"_s + lang.code);
        const QString code = lang.code;
        connect(item, &QAction::triggered, this, [this, skillId, code] { Q_EMIT chosen(skillId, code); });
    }
}

void ActionBar::highlight(const QString& base)
{
    if (ChipButton* b = m_buttons.value(base)) {
        b->setChecked(true);
        b->setFocus();
        updateArrows();
    }
}

void ActionBar::setCurrent(const SkillInfo& skill, const QString& targetLang)
{
    const QString base = m_catalog->kindOf(skill.id) == SkillKind::FullReply ? u"reply"_s : skill.base;
    ChipButton* b = m_buttons.value(base);
    if (b == nullptr) {
        return;
    }
    b->setChecked(true);
    if (m_catalog->picksLanguage(base) && !targetLang.isEmpty()) {
        b->setLabel(tr("%1: %2").arg(m_catalog->actionTitle(base), languageName(m_detector, targetLang)));
    } else if (b->menu() != nullptr) {
        b->setLabel(tr("%1: %2").arg(m_catalog->actionTitle(base), SkillCatalog::presetTitle(skill)));
    }
    updateArrows();
}

void ActionBar::setTextLanguage(std::function<QString()> currentLanguage)
{
    m_textLanguage = std::move(currentLanguage);
}

void ActionBar::applyTheme(const Tokens& t)
{
    m_tokens = t;
    updateArrows();
}

void ActionBar::updateArrows()
{
    for (ChipButton* b : std::as_const(m_buttons)) {
        if (b->menu() != nullptr) {
            const QColor colour = b->isChecked() ? QColor(Qt::white) : m_tokens.muted;
            b->setTrailingIcon(Theme::icon(u"down"_s, colour, kArrowSize));
        }
    }
}

} // namespace paraphrase::app

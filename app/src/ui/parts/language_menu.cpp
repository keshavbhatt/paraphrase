#include "ui/parts/language_menu.h"

#include "ui/common/menu_icons.h"

#include <QKeyEvent>
#include <QListWidget>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QWidgetAction>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kRowHeight = 32; // QMenu::item in theme.cpp: 6 px padding around the text
constexpr int kMaxRows = 12;   // then it scrolls
constexpr int kNativeGap = 32; // between a language's name and its own name
constexpr int kSidePad = 10;
constexpr int kNativePx = 12;
constexpr int kCaptionPx = 11;
constexpr int kNativeRole = Qt::UserRole + 1;
constexpr int kCaptionRole = Qt::UserRole + 2;
constexpr int kActionRole = Qt::UserRole + 3;

/// A language row: the badge, the name and, muted at the right edge, its own name; or a
/// heading in small capitals (.cap in mock.css).
class RowDelegate : public QStyledItemDelegate
{
public:
    explicit RowDelegate(const Tokens& tokens, QObject* parent)
        : QStyledItemDelegate(parent)
        , m_tokens(tokens)
    {}

    void paint(QPainter* p, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        const QString caption = index.data(kCaptionRole).toString();
        if (!caption.isEmpty()) {
            QFont caps = option.font;
            caps.setPixelSize(kCaptionPx);
            caps.setWeight(QFont::Bold);
            caps.setCapitalization(QFont::AllUppercase);
            caps.setLetterSpacing(QFont::PercentageSpacing, 106);
            p->save();
            p->setFont(caps);
            p->setPen(m_tokens.muted);
            p->drawText(option.rect.adjusted(kSidePad, 4, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, caption);
            p->restore();
            return;
        }
        QStyledItemDelegate::paint(p, option, index);
        const QString native = index.data(kNativeRole).toString();
        if (!native.isEmpty()) {
            QFont small = option.font;
            small.setPixelSize(kNativePx);
            p->save();
            p->setFont(small);
            p->setPen(m_tokens.muted);
            p->drawText(option.rect.adjusted(0, 0, -kSidePad, 0), Qt::AlignRight | Qt::AlignVCenter, native);
            p->restore();
        }
    }

    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        QFont small = option.font;
        small.setPixelSize(kNativePx);
        const int native = QFontMetrics(small).horizontalAdvance(index.data(kNativeRole).toString());
        size.setWidth(size.width() + (native > 0 ? kNativeGap + native : 0) + kSidePad);
        size.setHeight(kRowHeight);
        return size;
    }

    void setTokens(const Tokens& t) { m_tokens = t; }

private:
    Tokens m_tokens;
};

} // namespace

LanguageMenu::LanguageMenu(QWidget* parent)
    : QMenu(parent)
    , m_list(new QListWidget)
    , m_holder(new QWidgetAction(this))
{
    m_list->setObjectName(u"languageList"_s);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setIconSize(QSize(kMenuIconSize, kMenuIconSize));
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_list->setMouseTracking(true);
    m_list->setUniformItemSizes(true);
    m_list->setItemDelegate(new RowDelegate(m_tokens, m_list));
    m_holder->setDefaultWidget(m_list);
    addAction(m_holder);
    // The row under the pointer is the current one, as in a menu.
    connect(m_list, &QListWidget::itemEntered, m_list, [this](QListWidgetItem* item) {
        if ((item->flags() & Qt::ItemIsEnabled) != 0) {
            m_list->setCurrentItem(item);
        }
    });
    const auto pick = [this](const QListWidgetItem* item) {
        const int index = item->data(kActionRole).toInt();
        if (index < 0 || index >= m_languages.size()) {
            return;
        }
        QAction* language = m_languages.at(index);
        hide();
        language->trigger();
    };
    connect(m_list, &QListWidget::itemClicked, this, pick);
    connect(m_list, &QListWidget::itemActivated, this, pick); // Enter
}

void LanguageMenu::clearLanguages()
{
    m_list->clear();
    qDeleteAll(m_languages);
    m_languages.clear();
}

QAction* LanguageMenu::addLanguage(const QString& code, const QString& name, const QString& native,
                                   const QIcon& icon)
{
    auto* language = new QAction(name, this); // the menu's child, never shown as a row of its own
    language->setObjectName(u"lang-"_s + code);
    language->setIcon(icon);
    language->setProperty("nativeName", native);
    auto* item = new QListWidgetItem(icon, name, m_list);
    item->setData(kNativeRole, native == name ? QString() : native);
    item->setData(kActionRole, static_cast<int>(m_languages.size()));
    m_languages.append(language);
    return language;
}

void LanguageMenu::addCaption(const QString& text)
{
    auto* item = new QListWidgetItem(m_list);
    item->setData(kCaptionRole, text);
    item->setData(kActionRole, -1);
    item->setFlags(Qt::NoItemFlags); // the arrows pass over it
}

void LanguageMenu::finish()
{
    int width = 0;
    for (int i = 0; i < m_list->count(); ++i) {
        width = std::max(width, m_list->sizeHintForIndex(m_list->model()->index(i, 0)).width());
    }
    const int rows = std::min(m_list->count(), kMaxRows);
    const bool scrolls = m_list->count() > kMaxRows;
    const int scrollbar = scrolls ? 12 : 0;
    m_list->setFixedSize(width + scrollbar + (2 * kSidePad), (rows * kRowHeight) + 2);
}

void LanguageMenu::setTokens(const Tokens& t)
{
    m_tokens = t;
    static_cast<RowDelegate*>(m_list->itemDelegate())->setTokens(t);
}

void LanguageMenu::showEvent(QShowEvent* event)
{
    QMenu::showEvent(event);
    // The list takes the keys: arrows, Enter; Esc still closes the menu.
    setActiveAction(m_holder);
    m_list->setFocus();
    for (int i = 0; i < m_list->count(); ++i) {
        if ((m_list->item(i)->flags() & Qt::ItemIsEnabled) != 0) {
            m_list->setCurrentRow(i);
            break;
        }
    }
    m_list->scrollToTop();
}

} // namespace paraphrase::app

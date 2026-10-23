#include "ui/main/bars.h"

#include "ui/common/widgets.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kMarkSize = 18;
constexpr int kSmallIcon = 14;
constexpr int kDotSize = 8;

} // namespace

HeaderBar::HeaderBar(QWidget* parent)
    : QFrame(parent)
    , m_mark(new QLabel)
    , m_lock(new QLabel)
{
    setObjectName(u"header"_s);
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(14, 10, 14, 10);
    row->setSpacing(10);
    row->addWidget(m_mark);
    row->addWidget(makeLabel(u"<b>Paraphrase</b>"_s));
    row->addStretch();
    auto* privacy = new QHBoxLayout;
    privacy->setSpacing(5);
    privacy->addWidget(m_lock);
    privacy->addWidget(makeLabel(tr("On this device"), u"private"_s));
    row->addLayout(privacy);
    QPushButton* settings = makeButton(tr("Settings"), u"flat"_s);
    settings->setObjectName(u"settings"_s);
    row->addWidget(settings);
    connect(settings, &QPushButton::clicked, this, &HeaderBar::settingsClicked);
}

void HeaderBar::applyTheme(const Tokens& t)
{
    m_mark->setPixmap(Theme::icon(u"pen"_s, t.accent, kMarkSize).pixmap(kMarkSize, kMarkSize));
    m_lock->setPixmap(Theme::icon(u"lock"_s, t.privacy, kSmallIcon).pixmap(kSmallIcon, kSmallIcon));
}

NoticeBar::NoticeBar(QWidget* parent)
    : QFrame(parent)
    , m_icon(new QLabel)
    , m_text(makeLabel(QString()))
    , m_hide(makeButton(QString(), u"icon"_s))
{
    setObjectName(u"notice"_s);
    auto* row = new QHBoxLayout(this);
    // main-window.html .notice: padding 10px 16px, icon on the first line of the text.
    row->setContentsMargins(16, 10, 16, 10);
    row->setSpacing(10);
    m_icon->setContentsMargins(0, 2, 0, 0);
    row->addWidget(m_icon, 0, Qt::AlignTop);
    m_text->setObjectName(u"noticeText"_s);
    m_text->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_text->setWordWrap(true);
    m_text->setTextFormat(Qt::RichText);
    connect(m_text, &QLabel::linkActivated, this, &NoticeBar::linkClicked);
    row->addWidget(m_text, 1);
    m_hide->setObjectName(u"hideNotice"_s);
    m_hide->setProperty("flat", true);
    m_hide->setToolTip(tr("Hide this notice"));
    m_hide->setAccessibleName(tr("Hide this notice"));
    connect(m_hide, &QPushButton::clicked, this, &QWidget::hide);
    row->addWidget(m_hide, 0, Qt::AlignTop);
}

void NoticeBar::setNotice(const QString& text, const QString& link)
{
    const QString escaped = text.toHtmlEscaped();
    m_text->setText(link.isEmpty() ? escaped
                                   : u"%1 <a href=\"#\">%2</a>"_s.arg(escaped, link.toHtmlEscaped()));
    setVisible(!text.isEmpty());
}

void NoticeBar::applyTheme(const Tokens& t)
{
    m_icon->setPixmap(Theme::icon(u"info"_s, t.warning, kSmallIcon).pixmap(kSmallIcon, kSmallIcon));
    m_hide->setIcon(Theme::icon(u"x"_s, t.warning, kSmallIcon));
}

StatusBar::StatusBar(QWidget* parent)
    : QFrame(parent)
    , m_dot(new QLabel)
    , m_text(makeLabel(tr("Starting...")))
{
    setObjectName(u"statusBar"_s);
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(16, 7, 16, 7);
    row->setSpacing(10);
    m_dot->setObjectName(u"dot"_s);
    m_dot->setFixedSize(kDotSize, kDotSize);
    row->addWidget(m_dot);
    m_text->setObjectName(u"statusText"_s);
    row->addWidget(m_text, 1); // the spare width, so a longer model line is never cut
    row->addWidget(makeLabel(tr("Nothing leaves this computer")));
}

void StatusBar::showModels(const QJsonObject& status, const QJsonArray& models)
{
    // The loaded model with its memory; else the first installed one; else what to do.
    QString line;
    for (const auto& v : models) {
        const QJsonObject m = v.toObject();
        if (m.value(u"role"_s).toString() != u"model"_s || !m.value(u"installed"_s).toBool()) {
            continue;
        }
        const QString tier = m.value(u"tiers"_s).toArray().first().toString();
        const QString entry =
            tr("%1 (%2)").arg(m.value(u"name"_s).toString(), tier == u"fast"_s ? tr("fast") : tr("quality"));
        if (m.value(u"id"_s).toString() == status.value(u"model"_s).toString()) {
            line = tr("%1 · about %2 GB memory in use").arg(entry).arg(m.value(u"memory_gb"_s).toDouble());
            break;
        }
        if (line.isEmpty()) {
            line = tr("%1 ready").arg(entry);
        }
    }
    if (line.isEmpty()) {
        m_text->setText(tr("No model is installed yet: open Settings to download one"));
        setLevel(Level::Warning);
        return;
    }
    m_text->setText(line);
    setLevel(Level::Ok);
}

void StatusBar::showProblem(const QString& text)
{
    m_text->setText(text);
    setLevel(Level::Danger);
}

void StatusBar::applyTheme(const Tokens& t)
{
    m_tokens = t;
    setLevel(m_level);
}

void StatusBar::setLevel(Level level)
{
    m_level = level;
    QColor colour = m_tokens.privacy;
    if (level == Level::Warning) {
        colour = m_tokens.warning;
    } else if (level == Level::Danger) {
        colour = m_tokens.danger;
    }
    m_dot->setStyleSheet(u"background: %1; border-radius: 4px;"_s.arg(colour.name()));
}

} // namespace paraphrase::app

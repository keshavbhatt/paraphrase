#include "popup/popup_window.h"

#include "popup/popup_pages.h"
#include "ui/common/widgets.h"

#include <QCloseEvent>
#include <QKeyEvent>
#include <QPushButton>
#include <QScreen>
#include <QStackedWidget>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kWidth = 440;
constexpr int kOffset = 12; ///< from the cursor

} // namespace

PopupWindow::PopupWindow(const util::LanguageDetector& detector, QWidget* parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    , m_pages(new QStackedWidget)
    , m_actions(new PopupActionsPage(detector))
    , m_result(new PopupResultPage(detector))
{
    setObjectName(u"popupWindow"_s);
    setWindowTitle(tr("Paraphrase"));
    setAttribute(Qt::WA_TranslucentBackground); // the rounded corners of the frame
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    auto* frame = new QFrame;
    frame->setObjectName(u"popup"_s);
    auto* inner = new QVBoxLayout(frame);
    inner->setContentsMargins(0, 0, 0, 0);
    inner->addWidget(m_pages);
    outer->addWidget(frame);
    m_pages->addWidget(m_actions);
    m_pages->addWidget(m_result);
    setFixedWidth(kWidth);
    connect(m_actions, &PopupActionsPage::closeRequested, this, &QWidget::close);
    connect(m_result, &PopupResultPage::closeRequested, this, &QWidget::close);
    connect(m_result, &PopupResultPage::back, this, &PopupWindow::showActions);
    applyTheme();
}

void PopupWindow::showActions()
{
    showOnly(m_pages, m_actions);
    adjustSize();
}

void PopupWindow::showResult()
{
    showOnly(m_pages, m_result);
    adjustSize();
}

void PopupWindow::openNear(const QPoint& point)
{
    adjustSize();
    QPoint at = point + QPoint(kOffset, kOffset);
    if (const QScreen* screen = QGuiApplication::screenAt(point)) {
        const QRect area = screen->availableGeometry();
        at.setX(std::clamp(at.x(), area.left(), std::max(area.left(), area.right() - width())));
        at.setY(std::clamp(at.y(), area.top(), std::max(area.top(), area.bottom() - height())));
    }
    move(at);
    show();
    raise();
    activateWindow();
}

void PopupWindow::applyTheme()
{
    m_tokens = Theme::current();
    setStyleSheet(Theme::styleSheet(m_tokens));
    m_actions->applyTheme(m_tokens);
    m_result->applyTheme(m_tokens);
}

void PopupWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    // Enter on a result is Replace or Insert reply (popup-proofread.html, note 4).
    const bool enter = event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter;
    if (enter && m_pages->currentWidget() == m_result) {
        if (auto* insert = m_result->findChild<QPushButton*>(u"insert"_s);
            insert != nullptr && insert->isEnabled()) {
            insert->click();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

void PopupWindow::changeEvent(QEvent* event)
{
    if ((event->type() == QEvent::ThemeChange || event->type() == QEvent::PaletteChange) &&
        Theme::current().dark != m_tokens.dark) {
        applyTheme();
    }
    QWidget::changeEvent(event);
}

void PopupWindow::closeEvent(QCloseEvent* event)
{
    Q_EMIT closed();
    QWidget::closeEvent(event);
}

} // namespace paraphrase::app

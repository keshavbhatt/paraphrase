#pragma once

#include <QFrame>
#include <QIcon>
#include <QList>
#include <QString>

class QHBoxLayout;
class QLabel;
class QPushButton;
class QStackedWidget;
class QVBoxLayout;

namespace paraphrase::app {

/// Small building blocks every surface uses, styled by Theme through properties.
[[nodiscard]] QLabel* makeLabel(const QString& text, const QString& role = {});
[[nodiscard]] QPushButton* makeButton(const QString& text, const QString& kind = {});
/// Makes `button` as wide as the widest of `texts` (with `icon`, when given), so swapping its
/// text on a click, such as Retry to Stop, never moves the buttons beside it.
void reserveWidth(QPushButton* button, const QStringList& texts, const QIcon& icon = {});
/// A settings page's title and its intro, 6 px apart (settings mocks: h1 and intro in a .stack).
[[nodiscard]] QVBoxLayout* pageHeading(const QString& title, const QString& intro);
/// Shows `page` and lets only it count for the stack's size: a QStackedWidget is otherwise
/// as tall as its tallest page, which leaves a popup with empty space under a short one.
void showOnly(QStackedWidget* stack, QWidget* page);

/// The mocks' `.seg`: two or more exclusive choices in one rounded box.
class SegmentedControl : public QFrame
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(SegmentedControl)

public:
    struct Item
    {
        QString name; ///< object name of the choice's button, for tests and accessibility
        QString text;
    };

    explicit SegmentedControl(const QList<Item>& items, QWidget* parent = nullptr);
    ~SegmentedControl() override = default;

    [[nodiscard]] int current() const;
    void setCurrent(int index);

Q_SIGNALS:
    void changed(int index); ///< the user picked another choice

private:
    QList<QPushButton*> m_buttons;
};

/// A pane of the main window (`.pane`): a head row, a body and an optional foot, separated by
/// lines, in a rounded frame.
class Pane : public QFrame
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Pane)

public:
    explicit Pane(QWidget* parent = nullptr);
    ~Pane() override = default;

protected:
    [[nodiscard]] QHBoxLayout* head() const { return m_head; }
    void setBody(QWidget* body);
    /// The foot row, created on first use.
    [[nodiscard]] QHBoxLayout* footRow();
    [[nodiscard]] QFrame* foot() const { return m_foot; }

private:
    QVBoxLayout* m_layout = nullptr;
    QHBoxLayout* m_head = nullptr;
    QFrame* m_foot = nullptr;
    QHBoxLayout* m_footRow = nullptr;
};

} // namespace paraphrase::app

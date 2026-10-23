#pragma once

#include "ui/common/theme.h"

#include <QElapsedTimer>
#include <QStringList>
#include <QTimer>
#include <QWidget>

namespace paraphrase::app {

/// The working pill (main-window-busy.html): the step in words and the seconds so far, with a
/// dot that pulses, on the accent's soft colour. It keeps the width of its longest step, so
/// nothing beside it moves when the step changes.
class BusyPill : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(BusyPill)

public:
    explicit BusyPill(QWidget* parent = nullptr);
    ~BusyPill() override = default;

    /// Every step it can show, for its width.
    void setSteps(const QStringList& steps);
    /// Shows the pill and counts from zero.
    void start(const QString& step);
    /// Another step of the same run: the seconds go on.
    void setStep(const QString& step);
    void stop();
    [[nodiscard]] QString step() const { return m_step; }
    void applyTheme(const Tokens& t);

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    [[nodiscard]] QFont stepFont() const;
    [[nodiscard]] QString seconds() const;

    QStringList m_steps;
    QString m_step;
    QElapsedTimer m_clock;
    QTimer m_tick;
    Tokens m_tokens;
};

} // namespace paraphrase::app

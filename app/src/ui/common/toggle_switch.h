#pragma once

#include "ui/common/theme.h"

#include <QAbstractButton>

namespace paraphrase::app {

/// An on/off switch (the mocks' `.switch`), keyboard accessible like a check box.
class ToggleSwitch : public QAbstractButton
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ToggleSwitch)

public:
    explicit ToggleSwitch(QWidget* parent = nullptr);
    ~ToggleSwitch() override = default;

    [[nodiscard]] QSize sizeHint() const override { return {38, 22}; }
    void applyTheme(const Tokens& t);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    Tokens m_tokens;
};

} // namespace paraphrase::app

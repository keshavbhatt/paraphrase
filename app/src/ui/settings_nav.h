#pragma once

#include <QStringList>
#include <QWidget>

class QButtonGroup;

namespace paraphrase::app {

/// The settings page list (settings-models.html, left column): one entry per page, the open
/// one highlighted.
class SettingsNav : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(SettingsNav)

public:
    explicit SettingsNav(const QStringList& pages, QWidget* parent = nullptr);
    ~SettingsNav() override = default;

    void setCurrent(int page);

Q_SIGNALS:
    void pageChosen(int page);

private:
    QButtonGroup* m_group;
};

} // namespace paraphrase::app

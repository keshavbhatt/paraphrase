#pragma once

#include "theme.h"

#include <QWidget>

class QVBoxLayout;

namespace paraphrase::app {

class ModelsController;

/// Settings, About: the version, the licence, what Paraphrase is built with and the licence of
/// each installed model.
class AboutPage : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(AboutPage)

public:
    explicit AboutPage(ModelsController& models, QWidget* parent = nullptr);
    ~AboutPage() override = default;

    /// Link colours for the current theme.
    void setTheme(const Tokens& tokens);

private:
    void reload();

    ModelsController& m_models;
    QVBoxLayout* m_modelLines = nullptr;
    Tokens m_tokens;
};

} // namespace paraphrase::app

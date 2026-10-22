#pragma once

#include "theme.h"

#include <QFrame>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

class QLabel;
class QPushButton;

namespace paraphrase::app {

/// The header band (`.mwhead`): the mark, the privacy line, Settings.
class HeaderBar : public QFrame
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(HeaderBar)

public:
    explicit HeaderBar(QWidget* parent = nullptr);
    ~HeaderBar() override = default;
    void applyTheme(const Tokens& t);

Q_SIGNALS:
    void settingsClicked();

private:
    QLabel* m_mark = nullptr;
    QLabel* m_lock = nullptr;
};

/// The notice band for what the desktop does not offer (ADR-013): never an error, can be hidden.
class NoticeBar : public QFrame
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(NoticeBar)

public:
    explicit NoticeBar(QWidget* parent = nullptr);
    ~NoticeBar() override = default;
    /// The notice text and an optional link at its end; an empty text hides the band.
    void setNotice(const QString& text, const QString& link = {});
    void applyTheme(const Tokens& t);

Q_SIGNALS:
    void linkClicked();

private:
    QLabel* m_icon = nullptr;
    QLabel* m_text = nullptr;
    QPushButton* m_hide = nullptr;
};

/// The status band: which model answers, and the privacy promise.
class StatusBar : public QFrame
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(StatusBar)

public:
    explicit StatusBar(QWidget* parent = nullptr);
    ~StatusBar() override = default;

    /// "Gemma 4 E2B (fast) · about 4.6 GB memory in use", from Status() and ListModels().
    void showModels(const QJsonObject& status, const QJsonArray& models);
    void showProblem(const QString& text);
    void applyTheme(const Tokens& t);

private:
    enum class Level : std::uint8_t
    {
        Ok,
        Warning,
        Danger,
    };
    void setLevel(Level level);

    QLabel* m_dot = nullptr;
    QLabel* m_text = nullptr;
    Level m_level = Level::Ok;
    Tokens m_tokens;
};

} // namespace paraphrase::app

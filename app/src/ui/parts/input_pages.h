#pragma once

#include "ui/common/theme.h"
#include "ui/core/skill_catalog.h"

#include <QList>
#include <QString>
#include <QVariantMap>
#include <QWidget>

#include <utility>

class QPlainTextEdit;
class QPushButton;

namespace paraphrase::app {

class SegmentedControl;

/// What a kind of action needs from the user, as one page of the input pane. Each page owns
/// its fields and turns them into the request's inputs (DOCS/api.md, `input`).
class InputPage : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(InputPage)

public:
    explicit InputPage(QWidget* parent = nullptr)
        : QWidget(parent)
    {}
    ~InputPage() override = default;

    /// The text the page is about (your text, the message): its words and language are shown.
    [[nodiscard]] virtual QString subject() const = 0;
    virtual void setSubject(const QString& text) = 0;
    /// The request's inputs for this skill; empty when the page is not filled enough to run.
    [[nodiscard]] virtual QVariantMap input(const SkillInfo& skill, const QString& targetLang) const = 0;
    virtual void clear() = 0;

Q_SIGNALS:
    void edited();
};

/// Your text: proofread, tones, summarize, format, translate.
class TextInput : public InputPage
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TextInput)

public:
    explicit TextInput(QWidget* parent = nullptr);
    ~TextInput() override = default;

    [[nodiscard]] QString subject() const override;
    void setSubject(const QString& text) override;
    [[nodiscard]] QVariantMap input(const SkillInfo& skill, const QString& targetLang) const override;
    void clear() override;

private:
    QPlainTextEdit* m_text = nullptr;
};

/// The message you got and, optionally, the reply you started (main-window-reply.html).
class QuickReplyInput : public InputPage
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QuickReplyInput)

public:
    explicit QuickReplyInput(QWidget* parent = nullptr);
    ~QuickReplyInput() override = default;

    [[nodiscard]] QString subject() const override;
    void setSubject(const QString& text) override;
    [[nodiscard]] QVariantMap input(const SkillInfo& skill, const QString& targetLang) const override;
    void clear() override;

private:
    QPlainTextEdit* m_message = nullptr;
    QPlainTextEdit* m_draft = nullptr;
};

/// The message, what you want to say, email or chat message, and a tone
/// (main-window-reply-draft.html). The kind and the tone are remembered.
class FullReplyInput : public InputPage
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(FullReplyInput)

public:
    explicit FullReplyInput(QWidget* parent = nullptr);
    ~FullReplyInput() override = default;

    [[nodiscard]] QString subject() const override;
    void setSubject(const QString& text) override;
    [[nodiscard]] QVariantMap input(const SkillInfo& skill, const QString& targetLang) const override;
    void clear() override;

    /// The tones to offer, as (tone, title) from the Tone action's presets.
    void setTones(const QList<std::pair<QString, QString>>& tones);
    void applyTheme(const Tokens& t);
    [[nodiscard]] bool chat() const;

Q_SIGNALS:
    void kindChanged(); ///< email or chat message

private:
    void chooseTone(const QString& tone, const QString& title);

    QPlainTextEdit* m_message = nullptr;
    QPlainTextEdit* m_intent = nullptr;
    SegmentedControl* m_kind = nullptr;
    QPushButton* m_tone = nullptr;
    QString m_toneValue;
};

} // namespace paraphrase::app

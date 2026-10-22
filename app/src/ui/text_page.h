#pragma once

#include <QStringList>
#include <QWidget>

class QVBoxLayout;

namespace paraphrase::app {

/// A settings page that is a title and a few paragraphs: General, Shortcut and My actions say
/// what is coming; Privacy says what is true. The first paragraph reads as body text, the rest
/// as notes.
class TextPage : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TextPage)

public:
    TextPage(const QString& title, const QStringList& paragraphs, QWidget* parent = nullptr);
    ~TextPage() override = default;

protected:
    [[nodiscard]] QVBoxLayout* column() const { return m_column; }

private:
    QVBoxLayout* m_column;
};

} // namespace paraphrase::app

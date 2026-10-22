#pragma once

#include <QFrame>

namespace paraphrase::app {

/// The Languages card (settings-models.html, note 5): the tested languages, then the best
/// effort ones. Tested means a full eval set and a passed floor check (DOCS/models.md).
class LanguagesSection : public QFrame
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(LanguagesSection)

public:
    explicit LanguagesSection(QWidget* parent = nullptr);
    ~LanguagesSection() override = default;
};

} // namespace paraphrase::app

#pragma once

#include "ui/common/theme.h"

#include <QWidget>

class QComboBox;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

namespace paraphrase::app {

namespace history {
class Keeper;
}
class FlowLayout;
class ToggleSwitch;

/// Settings, Privacy (settings-privacy.html): the history switch, how long entries stay, the
/// apps never kept, what is kept and where, and Delete all, asked on the page.
class PrivacyPage : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(PrivacyPage)

public:
    explicit PrivacyPage(history::Keeper& keeper, QWidget* parent = nullptr);
    ~PrivacyPage() override = default;

    /// Shows the current state (the page is opened, or the history changed).
    void reload();
    void applyTheme(const Tokens& t);

private:
    enum class Asking : quint8
    {
        Nothing,
        DeleteAll, ///< "Delete all history" was clicked
        AfterOff,  ///< history was just turned off with entries kept
    };
    void wire();
    void addOptions(QVBoxLayout* onRows);
    void buildFacts();
    [[nodiscard]] QWidget* buildConfirm();
    void switched(bool on);
    void ask(Asking what);
    void addApp();
    void showApps();

    history::Keeper& m_keeper;
    ToggleSwitch* m_switch;
    QLabel* m_switchDetail = nullptr;
    QLabel* m_problem = nullptr;
    QFrame* m_switchRow = nullptr;
    QWidget* m_onRows = nullptr;
    QComboBox* m_keepFor = nullptr;
    QWidget* m_apps = nullptr;
    FlowLayout* m_appsFlow = nullptr;
    QPushButton* m_addApp = nullptr;
    QLineEdit* m_appName = nullptr;
    QWidget* m_factsRow = nullptr;
    QLabel* m_kept = nullptr;
    QLabel* m_where = nullptr;
    QLabel* m_lock = nullptr;
    QLabel* m_size = nullptr;
    QPushButton* m_deleteAll = nullptr;
    QFrame* m_confirm = nullptr;
    QLabel* m_question = nullptr;
    QLabel* m_questionDetail = nullptr;
    Asking m_asking = Asking::Nothing;
    Tokens m_tokens;
};

} // namespace paraphrase::app

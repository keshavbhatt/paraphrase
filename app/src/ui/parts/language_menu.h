#pragma once

#include "ui/common/theme.h"

#include <QMenu>

class QListWidget;
class QWidgetAction;

namespace paraphrase::app {

/// The languages to translate or write into (menus.html): each with its script and its own
/// name, the untested ones under "Best effort". A menu holding one list that scrolls, about
/// twelve languages tall, so it never covers the screen (owner, 2026-10-23); Qt's own menus
/// scroll only once they are taller than the screen.
///
/// Every language is also a QAction of the menu (not shown, named "lang-<code>"): picking it
/// in the list triggers it, so callers connect to actions as with any menu.
class LanguageMenu : public QMenu
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(LanguageMenu)

public:
    explicit LanguageMenu(QWidget* parent = nullptr);
    ~LanguageMenu() override = default;

    void clearLanguages();
    QAction* addLanguage(const QString& code, const QString& name, const QString& native, const QIcon& icon);
    /// A heading between languages ("BEST EFFORT").
    void addCaption(const QString& text);
    /// Fits the list to its rows, up to the cap; call after adding.
    void finish();
    void setTokens(const Tokens& t);

protected:
    void showEvent(QShowEvent* event) override;

private:
    QListWidget* m_list = nullptr;
    QWidgetAction* m_holder = nullptr;
    QList<QAction*> m_languages;
    Tokens m_tokens;
};

} // namespace paraphrase::app

#pragma once

#include "daemon_client.h"
#include "integration/backends.h"
#include "ui/skill_catalog.h"
#include "util/language_detector.h"

#include <QJsonObject>
#include <QObject>
#include <QString>

#include <memory>

namespace paraphrase::app {

class Integrations;
class PopupWindow;
class RequestRunner;

/// The popup flow (PLAN 3.2): capture the text the user means, open the popup with the
/// actions, read ahead for the likely one, run the chosen one, and put the result back only
/// when the user clicks Replace or Insert reply. Nothing is typed or sent otherwise
/// (CLAUDE.md), and a result goes to the clipboard when typing is not possible.
class PopupController : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(PopupController)

public:
    PopupController(const Integrations& integrations, QObject* parent = nullptr);
    ~PopupController() override;

    /// Captures the text now and opens the popup near the cursor.
    void show();
    /// Opens the popup on this text (tests, and the main window's hand-over in the other way).
    void showText(const CapturedText& text);
    [[nodiscard]] PopupWindow* window() const { return m_window.get(); }

Q_SIGNALS:
    /// "Open in window": the main window takes the text (ADR-013).
    void openInWindow(const QString& text);
    void finished(); ///< a run ended, with a result or a message (tests)

private:
    [[nodiscard]] bool ensureCatalog();
    void connectWindow();
    void readAhead();
    void run(const QString& skillId, const QString& targetLang);
    void onFinished(const QJsonObject& result);
    void onFailed(const QString& code, const QString& message);
    void insert();
    void copy();
    [[nodiscard]] QString likelyBase() const;

    const Integrations& m_integrations;
    client::DaemonClient m_client;
    util::LanguageDetector m_detector;
    SkillCatalog m_catalog;
    RequestRunner* m_runner = nullptr;
    std::unique_ptr<PopupWindow> m_window;
    CapturedText m_text;
    QString m_skill;
    QString m_targetLang;
    QString m_streamed;
};

} // namespace paraphrase::app

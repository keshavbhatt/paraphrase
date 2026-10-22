#pragma once

#include <QObject>
#include <QString>

namespace paraphrase::app {

/// The running app on the session bus (com.ktechpit.paraphrase.App), so `paraphrase-app
/// --popup` bound to a desktop shortcut reaches it instead of starting a second copy. D-Bus
/// only: no TCP (CLAUDE.md).
class AppService : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.ktechpit.paraphrase.App")
    Q_DISABLE_COPY_MOVE(AppService)

public:
    static constexpr const char* kService = "com.ktechpit.paraphrase.App";
    static constexpr const char* kPath = "/com/ktechpit/paraphrase/App";

    explicit AppService(QObject* parent = nullptr)
        : QObject(parent)
    {}
    ~AppService() override = default;

    /// Registers the service; false when another copy already runs it.
    [[nodiscard]] bool registerOnBus();
    /// Asks the copy that runs the service to do `method`; false when none answers.
    [[nodiscard]] static bool forward(const QString& method);

public Q_SLOTS:
    // D-Bus methods are named as D-Bus names them (DOCS/api.md).
    // NOLINTBEGIN(readability-identifier-naming)
    Q_SCRIPTABLE void ShowPopup() { Q_EMIT popupRequested(); }
    Q_SCRIPTABLE void ShowWindow() { Q_EMIT windowRequested(); }
    // NOLINTEND(readability-identifier-naming)

Q_SIGNALS:
    void popupRequested();
    void windowRequested();
};

} // namespace paraphrase::app

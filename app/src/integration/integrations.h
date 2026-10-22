#pragma once

#include "backends.h"

#include <QList>

#include <memory>
#include <vector>

namespace paraphrase::app {

/// The ranked backends for this session, best first (PLAN 6): the first available one of
/// each kind is used, and the list says what works for Settings and the notice. The
/// clipboard and "bind this command" are always last, so something always works.
class Integrations
{
public:
    Integrations(std::vector<std::unique_ptr<CaptureBackend>> capture,
                 std::vector<std::unique_ptr<InsertBackend>> insert, std::vector<ShortcutBackend*> shortcuts);
    /// The backends this build knows, for the running session.
    [[nodiscard]] static Integrations forThisSession(QObject* owner);

    [[nodiscard]] CaptureBackend& capture() const;
    [[nodiscard]] InsertBackend& insert() const;
    [[nodiscard]] ShortcutBackend& shortcut() const;
    /// True when the shortcut is a real global one, not a command to bind by hand.
    [[nodiscard]] bool hasGlobalShortcut() const;
    [[nodiscard]] QList<BackendStatus> status() const;

private:
    std::vector<std::unique_ptr<CaptureBackend>> m_capture;
    std::vector<std::unique_ptr<InsertBackend>> m_insert;
    std::vector<ShortcutBackend*> m_shortcuts; ///< QObjects owned by their parent
};

} // namespace paraphrase::app

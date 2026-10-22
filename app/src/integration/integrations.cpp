#include "integrations.h"

#include "clipboard_backends.h"

#include <algorithm>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

template <typename T>
T& firstAvailable(const std::vector<std::unique_ptr<T>>& backends)
{
    const auto it = std::ranges::find_if(backends, [](const auto& b) { return b->available(); });
    return it == backends.end() ? *backends.back() : **it;
}

} // namespace

Integrations::Integrations(std::vector<std::unique_ptr<CaptureBackend>> capture,
                           std::vector<std::unique_ptr<InsertBackend>> insert,
                           std::vector<ShortcutBackend*> shortcuts)
    : m_capture(std::move(capture))
    , m_insert(std::move(insert))
    , m_shortcuts(std::move(shortcuts))
{}

Integrations Integrations::forThisSession(QObject* owner)
{
    // Session-specific backends (X11, KDE Wayland) go before the fallbacks as they land.
    std::vector<std::unique_ptr<CaptureBackend>> capture;
    capture.push_back(std::make_unique<ClipboardCapture>());
    std::vector<std::unique_ptr<InsertBackend>> insert;
    insert.push_back(std::make_unique<ClipboardInsert>());
    std::vector<ShortcutBackend*> shortcuts{new CommandShortcut(owner)};
    return {std::move(capture), std::move(insert), std::move(shortcuts)};
}

CaptureBackend& Integrations::capture() const
{
    return firstAvailable(m_capture);
}

InsertBackend& Integrations::insert() const
{
    return firstAvailable(m_insert);
}

ShortcutBackend& Integrations::shortcut() const
{
    const auto it =
        std::ranges::find_if(m_shortcuts, [](const ShortcutBackend* b) { return b->available(); });
    return it == m_shortcuts.end() ? *m_shortcuts.back() : **it;
}

bool Integrations::hasGlobalShortcut() const
{
    return &shortcut() != m_shortcuts.back();
}

QList<BackendStatus> Integrations::status() const
{
    QList<BackendStatus> out;
    for (const ShortcutBackend* b : m_shortcuts) {
        out.append(
            {.kind = u"shortcut"_s, .name = b->name(), .works = b->available(), .detail = b->description()});
    }
    for (const auto& b : m_capture) {
        out.append({.kind = u"capture"_s, .name = b->name(), .works = b->available(), .detail = {}});
    }
    for (const auto& b : m_insert) {
        out.append({.kind = u"insert"_s, .name = b->name(), .works = b->available(), .detail = {}});
    }
    return out;
}

} // namespace paraphrase::app

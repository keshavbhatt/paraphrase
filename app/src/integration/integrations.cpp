#include "integrations.h"

#include "clipboard_backends.h"
#include "session_backends.h"

#include <QGuiApplication>

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
    SessionBackends b;
    // PARAPHRASE_NO_INTEGRATIONS=1 switches every desktop backend off: the standalone check
    // (ADR-013) and tests.
    if (qEnvironmentVariableIntValue("PARAPHRASE_NO_INTEGRATIONS") == 0) {
        const QString platform = QGuiApplication::platformName();
        if (platform == u"xcb"_s) {
            addX11Backends(b, owner);
        } else if (platform == u"wayland"_s) {
            addWaylandBackends(b, owner);
        }
    }
    // The fallbacks work everywhere, so something always does.
    b.capture.push_back(std::make_unique<ClipboardCapture>());
    b.insert.push_back(std::make_unique<ClipboardInsert>());
    b.shortcuts.push_back(new CommandShortcut(owner));
    return {std::move(b.capture), std::move(b.insert), std::move(b.shortcuts)};
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

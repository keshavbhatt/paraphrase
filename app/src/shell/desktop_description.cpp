#include "shell/desktop_description.h"

#include "integration/clipboard_backends.h"
#include "integration/integrations.h"

#include <QGuiApplication>
#include <QObject>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

QString whereToChange()
{
    const QString desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
    if (desktop.contains(u"KDE"_s)) {
        return QObject::tr("System Settings, Shortcuts");
    }
    if (desktop.contains(u"GNOME"_s)) {
        return QObject::tr("Settings, Keyboard, Shortcuts");
    }
    return {};
}

DesktopSupport::Step step(const QString& what, const QString& how, const QString& status, bool works)
{
    DesktopSupport::Step s;
    s.step = what;
    s.how = how;
    s.status = status;
    s.works = works;
    return s;
}

} // namespace

DesktopSupport describeDesktop(const Integrations& integrations)
{
    DesktopSupport s;
    s.globalShortcut = integrations.hasGlobalShortcut();
    s.shortcut = s.globalShortcut ? integrations.shortcut().keys() : QString();
    s.changeIt = whereToChange();
    const QString open = QObject::tr("Open over any app");
    s.steps.append(s.globalShortcut
                       ? step(open, QObject::tr("Shortcut from the desktop"), QObject::tr("Works"), true)
                       : step(open, QObject::tr("A shortcut you bind to the command above"),
                              QObject::tr("By hand"), false));
    const QString read = QObject::tr("Read the selected text");
    const bool readsSelection = dynamic_cast<const ClipboardCapture*>(&integrations.capture()) == nullptr;
    s.steps.append(
        readsSelection
            ? step(read, QObject::tr("The selection, without copying"), QObject::tr("Works"), true)
            : step(read, QObject::tr("What you copied: copy it first"), QObject::tr("Copy first"), false));
    const QString back = QObject::tr("Put the result back");
    const bool pastes = dynamic_cast<const ClipboardInsert*>(&integrations.insert()) == nullptr;
    const bool wayland = QGuiApplication::platformName() == u"wayland"_s;
    if (!pastes) {
        s.steps.append(
            step(back, QObject::tr("On the clipboard: you paste it"), QObject::tr("You paste"), false));
    } else if (wayland) {
        s.steps.append(step(back, QObject::tr("Pasted into the app, after you allow it once"),
                            QObject::tr("Asks once"), true));
    } else {
        s.steps.append(step(back, QObject::tr("Pasted into the app"), QObject::tr("Works"), true));
    }
    return s;
}

} // namespace paraphrase::app

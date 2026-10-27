#pragma once

#include <QString>
#include <QStringList>

namespace paraphrase::app {

/// How the desktop starts this app, per package format: `paraphrase` in a snap (snapd runs the
/// app named after the snap), `flatpak run com.ktechpit.paraphrase`, the AppImage's own path,
/// or `paraphrase-app` (its full path when it is not on PATH, as in a development build).
[[nodiscard]] QStringList launchCommand();

/// The same with arguments, as one line for a desktop file or a custom shortcut, each part
/// quoted where it needs to be (Desktop Entry Specification, "The Exec key").
[[nodiscard]] QString launchLine(const QStringList& arguments);

/// The name of the desktop file that starts this app, without `.desktop`: the window's app id,
/// which the taskbar matches to find the icon. snapd installs the file as `<snap>_<app>`, so a
/// snap uses that; everywhere else it is `com.ktechpit.paraphrase`.
[[nodiscard]] QString desktopFileName();

} // namespace paraphrase::app

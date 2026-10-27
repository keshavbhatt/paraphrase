#include "integration/launch_command.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QStandardPaths>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

QString quoted(const QString& argument)
{
    static const QString kReserved = u" \t\n\"'\\><~|&;$*?#()`"_s;
    const bool plain =
        !argument.isEmpty() && std::ranges::none_of(argument, [](QChar c) { return kReserved.contains(c); });
    if (plain) {
        return argument;
    }
    QString out = argument;
    for (const QString& c : {u"\\"_s, u"\""_s, u"`"_s, u"$"_s}) {
        out.replace(c, u"\\"_s + c);
    }
    return u"\""_s + out + u"\""_s;
}

} // namespace

QStringList launchCommand()
{
    if (const QString snap = qEnvironmentVariable("SNAP_NAME"); !snap.isEmpty()) {
        return {snap};
    }
    if (const QString id = qEnvironmentVariable("FLATPAK_ID"); !id.isEmpty()) {
        return {u"flatpak"_s, u"run"_s, id};
    }
    if (const QString appImage = qEnvironmentVariable("APPIMAGE"); !appImage.isEmpty()) {
        return {appImage};
    }
    const QString self = QCoreApplication::applicationFilePath();
    const QString onPath = QStandardPaths::findExecutable(u"paraphrase-app"_s);
    if (self.isEmpty() ||
        (!onPath.isEmpty() && QFileInfo(onPath).canonicalFilePath() == QFileInfo(self).canonicalFilePath())) {
        return {u"paraphrase-app"_s};
    }
    return {self};
}

QString launchLine(const QStringList& arguments)
{
    QStringList parts;
    for (const QString& part : launchCommand() + arguments) {
        parts << quoted(part);
    }
    return parts.join(u' ');
}

QString desktopFileName()
{
    if (const QString snap = qEnvironmentVariable("SNAP_INSTANCE_NAME", qEnvironmentVariable("SNAP_NAME"));
        !snap.isEmpty()) {
        return snap + u"_paraphrase"_s; // the app's name in snapcraft.yaml
    }
    return u"com.ktechpit.paraphrase"_s;
}

} // namespace paraphrase::app

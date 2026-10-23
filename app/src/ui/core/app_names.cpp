#include "ui/core/app_names.h"

#include <QFile>
#include <QHash>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

/// The "Name=" of a desktop file's [Desktop Entry] group (no localised names: the UI is English).
QString desktopName(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    bool entry = false;
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.startsWith(u'[')) {
            entry = line == u"[Desktop Entry]"_s;
        } else if (entry && line.startsWith(u"Name="_s)) {
            return line.mid(5).trimmed();
        }
    }
    return {};
}

} // namespace

QString appDisplayName(const QString& appId)
{
    if (appId.isEmpty()) {
        return {};
    }
    static QHash<QString, QString> cache;
    if (const auto it = cache.constFind(appId); it != cache.cend()) {
        return *it;
    }
    QString name;
    for (const QString& id : {appId, appId.toLower()}) {
        const QString path = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, id + u".desktop"_s);
        if (!path.isEmpty()) {
            name = desktopName(path);
            break;
        }
    }
    if (name.isEmpty()) {
        name = appId.section(u'.', -1);
        if (!name.isEmpty()) {
            name[0] = name.at(0).toUpper();
        }
    }
    cache.insert(appId, name);
    return name;
}

} // namespace paraphrase::app

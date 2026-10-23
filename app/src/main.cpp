// paraphrase-app: the desktop app (PLAN D1). Standalone first (ADR-013): the main window
// works with nothing from the desktop but a window and the clipboard; the popup, the tray
// and the global shortcut are accelerators.
//
//   paraphrase-app            open the window (or bring the running one up)
//   paraphrase-app --popup    open the popup on the selected text (bind this to a shortcut)
//   paraphrase-app --hidden   start in the tray (login autostart)

#include "paraphrase/version.h"
#include "shell/app_service.h"
#include "shell/application.h"
#include "ui/common/menu_icons.h"

#include <QApplication>
#include <QCommandLineParser>

using namespace Qt::StringLiterals;

int main(int argc, char* argv[])
{
    const QApplication app(argc, argv);
    QApplication::setApplicationName(u"Paraphrase"_s);
    QApplication::setOrganizationName(u"ktechpit"_s);
    QApplication::setApplicationVersion(
        QStringLiteral(PARAPHRASE_VERSION_STRING " (" PARAPHRASE_GIT_REVISION_STRING ")"));
    QApplication::setDesktopFileName(u"com.ktechpit.paraphrase"_s);
    // Menus draw their icons a little larger than the style's default (menus.html).
    QApplication::setStyle(new paraphrase::app::MenuIconStyle(QApplication::style()->name()));

    QCommandLineParser parser;
    parser.setApplicationDescription(u"Writing help that stays on this computer."_s);
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption popup(u"popup"_s, u"Open the popup on the selected text."_s);
    const QCommandLineOption hidden(u"hidden"_s, u"Start in the tray, without the window."_s);
    parser.addOptions({popup, hidden});
    parser.process(app);

    // One copy runs: a second one hands its request over and leaves.
    paraphrase::app::Application paraphraseApp;
    if (!paraphraseApp.service()->registerOnBus()) {
        const bool forwarded =
            paraphrase::app::AppService::forward(parser.isSet(popup) ? u"ShowPopup"_s : u"ShowWindow"_s);
        if (forwarded) {
            return 0;
        }
    }
    paraphraseApp.start(!parser.isSet(hidden) && !parser.isSet(popup));
    if (parser.isSet(popup)) {
        QMetaObject::invokeMethod(paraphraseApp.service(), &paraphrase::app::AppService::ShowPopup,
                                  Qt::QueuedConnection);
    }
    return QApplication::exec();
}

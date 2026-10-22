// paraphrase-app: the desktop windows (PLAN D1, ADR-013). Standalone first: everything works
// here with nothing from the desktop but a window and the clipboard.

#include "paraphrase/version.h"
#include "ui/app_flow.h"

#include <QApplication>

using namespace Qt::StringLiterals;

int main(int argc, char* argv[])
{
    const QApplication app(argc, argv);
    QApplication::setApplicationName(u"Paraphrase"_s);
    QApplication::setApplicationVersion(
        QStringLiteral(PARAPHRASE_VERSION_STRING " (" PARAPHRASE_GIT_REVISION_STRING ")"));
    QApplication::setDesktopFileName(u"com.ktechpit.paraphrase"_s);

    // First run opens the setup window; the main window follows (app_flow.h).
    paraphrase::app::AppFlow flow;
    flow.start();
    return QApplication::exec();
}

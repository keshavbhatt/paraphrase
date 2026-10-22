// paraphrase-app: the desktop window (PLAN D1, ADR-013). Standalone first: everything works
// here with nothing from the desktop but a window and the clipboard.

#include "paraphrase/version.h"
#include "ui/main_window.h"

#include <QApplication>
#include <QTimer>

using namespace Qt::StringLiterals;

int main(int argc, char* argv[])
{
    const QApplication app(argc, argv);
    QApplication::setApplicationName(u"Paraphrase"_s);
    QApplication::setApplicationVersion(
        QStringLiteral(PARAPHRASE_VERSION_STRING " (" PARAPHRASE_GIT_REVISION_STRING ")"));
    QApplication::setDesktopFileName(u"com.ktechpit.paraphrase"_s);

    paraphrase::app::MainWindow window;
    window.show();
    // Connect after the window is up: starting the daemon can take a moment.
    QTimer::singleShot(0, &window, &paraphrase::app::MainWindow::start);
    return QApplication::exec();
}

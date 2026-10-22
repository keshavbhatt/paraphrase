// paraphrase-daemon entry point. M1 step 1 proves the build and the llama.cpp link; the
// engine and the D-Bus API arrive in later steps.

#include "paraphrase/version.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTextStream>

#include <llama.h>

using namespace Qt::StringLiterals;

int main(int argc, char* argv[])
{
    const QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(u"paraphrase-daemon"_s);
    QCoreApplication::setApplicationVersion(
        QStringLiteral(PARAPHRASE_VERSION_STRING " (" PARAPHRASE_GIT_REVISION_STRING ")"));

    QCommandLineParser parser;
    parser.setApplicationDescription(u"Paraphrase inference daemon"_s);
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption systemInfo(u"system-info"_s, u"Print what llama.cpp detects on this CPU."_s);
    parser.addOption(systemInfo);
    parser.process(app);

    if (parser.isSet(systemInfo)) {
        llama_backend_init();
        QTextStream(stdout) << QString::fromUtf8(llama_print_system_info()) << '\n';
        llama_backend_free();
        return 0;
    }
    parser.showHelp(0);
}

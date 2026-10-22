// paraphrase: the command-line client of paraphrase-daemon (PLAN 4, DOCS/api.md).

#include "cli.h"
#include "paraphrase/version.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QSocketNotifier>

#include <array>
#include <csignal>
#include <cstdlib>
#include <sys/socket.h>
#include <unistd.h>

using namespace Qt::StringLiterals;
using paraphrase::cli::Cli;
using paraphrase::cli::Command;

namespace {

std::array<int, 2> g_signalPipe = {-1, -1}; // NOLINT: a signal handler can only reach a global

void onSigint(int /*signal*/)
{
    const char byte = 1;
    if (::write(g_signalPipe[1], &byte, 1) < 0) {
        std::_Exit(130); // cannot wake the event loop: stop now
    }
}

Command parse(const QCoreApplication& app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        u"Paraphrase: proofread, rewrite and reply, on this computer only.\n\n"
        "Commands:\n"
        "  proofread [TEXT]            correct spelling and grammar\n"
        "  rewrite [--tone T] [TEXT]   rewrite in a tone (see paraphrase skills; default professional)\n"
        "  reply [--draft D] [MESSAGE] suggest replies to a message\n"
        "  run SKILL [TEXT]            run any skill by its id\n"
        "  batch                       JSON lines in and out: {\"skill\", \"input\"} per line (eval)\n"
        "  skills                      list the skills\n"
        "  status                      the daemon, its model and this computer\n"
        "  models [list | download MODEL | pause MODEL | delete MODEL]\n\n"
        "Text comes from the arguments, or from standard input when there are none."_s);
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption tone({u"t"_s, u"tone"_s}, u"Rewrite tone: professional, casual, polite, ..."_s,
                                  u"tone"_s);
    const QCommandLineOption to(u"to"_s, u"Write the answer in this language (a code such as fr)."_s,
                                u"lang"_s);
    const QCommandLineOption draft(u"draft"_s, u"Reply: what you started typing."_s, u"text"_s);
    const QCommandLineOption context(u"context"_s, u"Run: the context input of the skill."_s, u"text"_s);
    const QCommandLineOption json(u"json"_s, u"Print the full result as JSON."_s);
    const QCommandLineOption noStream(u"no-stream"_s, u"Print the answer only when it is complete."_s);
    const QCommandLineOption model(u"model"_s, u"Run on this model (see paraphrase models)."_s, u"id"_s);
    parser.addOptions({tone, to, draft, context, json, noStream, model});
    parser.addPositionalArgument(u"command"_s, u"What to do (see above)."_s);
    parser.process(app);
    QStringList args = parser.positionalArguments();
    if (args.isEmpty()) {
        parser.showHelp(Cli::kExitUsage);
    }
    return {
        .name = args.takeFirst(),
        .args = args,
        .tone = parser.value(tone),
        .to = parser.value(to),
        .draft = parser.value(draft),
        .context = parser.value(context),
        .json = parser.isSet(json),
        .noStream = parser.isSet(noStream),
        .model = parser.value(model),
    };
}

} // namespace

int main(int argc, char* argv[])
{
    const QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(u"paraphrase"_s);
    QCoreApplication::setApplicationVersion(
        QStringLiteral(PARAPHRASE_VERSION_STRING " (" PARAPHRASE_GIT_REVISION_STRING ")"));
    Cli cli(parse(app));

    // Ctrl+C cancels the request instead of leaving it running in the daemon.
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, g_signalPipe.data()) == 0) {
        const auto* notifier = new QSocketNotifier(g_signalPipe[0], QSocketNotifier::Read, &cli);
        QObject::connect(notifier, &QSocketNotifier::activated, &cli, [&cli] { cli.interrupt(); });
        std::signal(SIGINT, onSigint);
    }
    return cli.exec();
}

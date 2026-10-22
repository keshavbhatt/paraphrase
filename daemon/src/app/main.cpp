// paraphrase-daemon: owns the model and serves the D-Bus API (PLAN D1, 5.1). Clients start it
// when it is not on the bus (ADR-010); it unloads the model and exits after quiet periods.
//
// Exit codes for the client that started it: 0 done (idle, or another daemon already owns the
// bus name), 1 bad setup (skills, arguments, socket), 3 no session bus, 4 the bus name may not
// be owned (a snap waiting for its name declaration). On 3 and 4 the client runs the daemon
// privately: `--private-socket PATH`, the same API on a peer-to-peer D-Bus connection.

#include "api/daemon_service.h"
#include "engine/llama_engine.h"
#include "paraphrase/version.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusConnectionInterface>
#include <QDBusServer>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>

#include <csignal>
#include <llama.h>
#include <optional>
#include <sys/prctl.h>
#include <sys/stat.h>

using namespace Qt::StringLiterals;
using paraphrase::api::DaemonService;

namespace {

constexpr int kExitOk = 0;
constexpr int kExitBadSetup = 1;
constexpr int kExitNoBus = 3;
constexpr int kExitNameNotAllowed = 4;

void err(const QString& message)
{
    QTextStream(stderr) << message << '\n';
}

/// The built-in skills: --skills-dir, $PARAPHRASE_SKILLS_DIR, `<prefix>/share/paraphrase/skills`
/// next to the binary (packages), or the source tree (development builds).
QString skillsDir(const QString& option)
{
    const QStringList candidates = {
        option,
        qEnvironmentVariable("PARAPHRASE_SKILLS_DIR"),
        QCoreApplication::applicationDirPath() + u"/../share/paraphrase/skills"_s,
        QStringLiteral(PARAPHRASE_SOURCE_SKILLS_DIR),
    };
    for (const QString& dir : candidates) {
        if (!dir.isEmpty() && QFileInfo(dir).isDir()) {
            return QDir(dir).canonicalPath();
        }
    }
    return {};
}

struct Options
{
    QString privateSocket;
    QString skillsDir;
    QString modelsDir;
    int idleUnloadS = 600;
    int idleExitS = 1800;
    bool systemInfo = false;
};

Options parse(const QCoreApplication& app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(u"Paraphrase inference daemon"_s);
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption systemInfo(u"system-info"_s, u"Print what llama.cpp detects on this CPU."_s);
    const QCommandLineOption privateSocket(
        u"private-socket"_s, u"Serve the API on a private Unix socket instead of the session bus."_s,
        u"path"_s);
    const QCommandLineOption skills(u"skills-dir"_s, u"Folder with the built-in skills."_s, u"dir"_s);
    const QCommandLineOption models(u"models-dir"_s, u"Folder with the models."_s, u"dir"_s);
    const QCommandLineOption idleUnload(
        u"idle-unload"_s, u"Unload the model after this many quiet seconds."_s, u"seconds"_s, u"600"_s);
    const QCommandLineOption idleExit(u"idle-exit"_s, u"Exit after this many quiet seconds."_s, u"seconds"_s,
                                      u"1800"_s);
    parser.addOptions({systemInfo, privateSocket, skills, models, idleUnload, idleExit});
    parser.process(app);
    return {
        .privateSocket = parser.value(privateSocket),
        .skillsDir = parser.value(skills),
        .modelsDir = parser.value(models),
        .idleUnloadS = parser.value(idleUnload).toInt(),
        .idleExitS = parser.value(idleExit).toInt(),
        .systemInfo = parser.isSet(systemInfo),
    };
}

std::optional<paraphrase::api::ServiceSetup> setupFrom(const Options& options)
{
    const QString skills = skillsDir(options.skillsDir);
    if (skills.isEmpty()) {
        err(u"No skills folder found; pass --skills-dir."_s);
        return std::nullopt;
    }
    auto registry = paraphrase::skills::SkillRegistry::load(skills);
    if (!registry.ok()) {
        err(registry.error());
        return std::nullopt;
    }
    return paraphrase::api::ServiceSetup{
        .skills = std::move(registry).value(),
        .catalog = paraphrase::models::ModelCatalog::builtIn(),
        .modelsRoot =
            options.modelsDir.isEmpty() ? paraphrase::models::ModelStore::defaultRoot() : options.modelsDir,
        .hardware = paraphrase::models::HardwareProbe::probe(),
        .engineFactory = [] { return std::make_unique<paraphrase::engine::LlamaEngine>(); },
        .urlResolver = {}, // Hugging Face at the pinned revision
        .idleUnloadMs = options.idleUnloadS * 1000,
        .idleExitMs = options.idleExitS * 1000,
    };
}

/// Private mode (ADR-010): only this user may connect, and the daemon goes when the client
/// that started it does.
int servePrivately(DaemonService& service, const QString& path)
{
    ::umask(0077);
    ::prctl(PR_SET_PDEATHSIG, SIGTERM);
    const QDBusServer server(u"unix:path="_s + path);
    if (!server.isConnected()) {
        err(u"Cannot listen on %1: %2"_s.arg(path, server.lastError().message()));
        return kExitBadSetup;
    }
    QObject::connect(&server, &QDBusServer::newConnection, &service,
                     [&service](const QDBusConnection& peer) { (void)service.exportOn(peer); });
    return QCoreApplication::exec();
}

int serveOnBus(DaemonService& service)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        err(u"No D-Bus session bus; run with --private-socket."_s);
        return kExitNoBus;
    }
    const QString name = QString::fromLatin1(DaemonService::kServiceName);
    if (!service.exportOn(bus) || !bus.registerService(name)) {
        if (bus.interface()->isServiceRegistered(name)) {
            return kExitOk; // another daemon serves already
        }
        err(u"Cannot own %1 on the session bus: %2"_s.arg(name, bus.lastError().message()));
        return kExitNameNotAllowed;
    }
    return QCoreApplication::exec();
}

} // namespace

int main(int argc, char* argv[])
{
    const QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(u"paraphrase-daemon"_s);
    QCoreApplication::setApplicationVersion(
        QStringLiteral(PARAPHRASE_VERSION_STRING " (" PARAPHRASE_GIT_REVISION_STRING ")"));
    const Options options = parse(app);

    if (options.systemInfo) {
        llama_backend_init();
        QTextStream(stdout) << QString::fromUtf8(llama_print_system_info()) << '\n';
        llama_backend_free();
        return kExitOk;
    }
    auto setup = setupFrom(options);
    if (!setup) {
        return kExitBadSetup;
    }
    DaemonService service(std::move(*setup));
    QObject::connect(&service, &DaemonService::idleTimeout, &app, &QCoreApplication::quit);
    return options.privateSocket.isEmpty() ? serveOnBus(service)
                                           : servePrivately(service, options.privateSocket);
}

#include "cli.h"

#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLocale>
#include <QTextStream>

#include <unistd.h>

using namespace Qt::StringLiterals;

namespace paraphrase::cli {
namespace {

const QString kErrorPrefix = u"com.ktechpit.paraphrase.Error."_s;

QTextStream& out()
{
    static QTextStream stream(stdout);
    return stream;
}

QTextStream& err()
{
    static QTextStream stream(stderr);
    return stream;
}

bool stdoutIsTerminal()
{
    return ::isatty(STDOUT_FILENO) == 1;
}

QJsonValue parseJson(const QString& text)
{
    const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
    return doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object());
}

QString size(qint64 bytes)
{
    return QLocale().formattedDataSize(bytes, 2, QLocale::DataSizeSIFormat);
}

} // namespace

Cli::Cli(Command command, QObject* parent)
    : QObject(parent)
    , m_command(std::move(command))
{}

QString Cli::inputText() const
{
    if (!m_command.args.isEmpty() && m_command.args != QStringList{u"-"_s}) {
        return m_command.args.join(u' ');
    }
    QFile in;
    if (!in.open(stdin, QIODevice::ReadOnly)) {
        return {};
    }
    return QString::fromUtf8(in.readAll()).trimmed();
}

int Cli::exec()
{
    const QString& name = m_command.name;
    static const QStringList kCommands = {
        u"proofread"_s, u"rewrite"_s, u"reply"_s, u"run"_s, u"batch"_s, u"skills"_s, u"status"_s, u"models"_s,
    };
    if (!kCommands.contains(name)) {
        err() << tr("Unknown command \"%1\"; see paraphrase --help.").arg(name) << Qt::endl;
        return kExitUsage;
    }
    if (const QString problem = m_client.connectToDaemon(); !problem.isEmpty()) {
        err() << problem << Qt::endl;
        return kExitFailed;
    }
    if (name == u"skills"_s) {
        return listSkills();
    }
    if (name == u"status"_s) {
        return status();
    }
    if (name == u"models"_s) {
        return models();
    }
    if (name == u"batch"_s) {
        return batch();
    }

    QVariantMap input;
    if (!m_command.to.isEmpty()) {
        input.insert(u"target_lang"_s, m_command.to);
    }
    QString skill = name;
    if (name == u"rewrite"_s) {
        skill = u"rewrite."_s + (m_command.tone.isEmpty() ? u"professional"_s : m_command.tone);
    } else if (name == u"run"_s) {
        if (m_command.args.isEmpty()) {
            err() << tr("run needs a skill id; see paraphrase skills.") << Qt::endl;
            return kExitUsage;
        }
        skill = m_command.args.takeFirst();
        if (!m_command.context.isEmpty()) {
            input.insert(u"context"_s, m_command.context);
        } else if (mainInput(skill) == u"context"_s) {
            // Like `reply`: the text given is the message to answer.
            input.insert(u"context"_s, inputText());
            return runSkill(skill, input);
        }
    }
    if (name == u"reply"_s) {
        input.insert(u"context"_s, inputText()); // the message to reply to
        if (!m_command.draft.isEmpty()) {
            input.insert(u"text"_s, m_command.draft);
        }
    } else {
        input.insert(u"text"_s, inputText());
    }
    return runSkill(skill, input);
}

int Cli::replyError(const QDBusMessage& reply)
{
    err() << reply.errorMessage() << Qt::endl;
    if (reply.errorName() == kErrorPrefix + u"NoModel"_s) {
        return kExitNoModel;
    }
    return reply.errorName() == kErrorPrefix + u"InvalidInput"_s ? kExitUsage : kExitFailed;
}

int Cli::runSkill(const QString& skillId, const QVariantMap& input)
{
    // Stream into a terminal; a pipe gets the finished answer only. JSON skills (reply)
    // print their answers one per line at the end.
    m_stream = stdoutIsTerminal() && !m_command.json && !m_command.noStream && skillId != u"reply"_s;
    QEventLoop loop;
    m_loop = &loop;
    connect(&m_client, &DaemonClient::token, &loop, [this](const QString& id, int, const QString& text) {
        if (m_stream && id == m_requestId) {
            out() << text << Qt::flush;
            m_streamed = true;
        }
    });
    connect(&m_client, &DaemonClient::retrying, &loop, [this](const QString& id, const QString& reason) {
        if (id == m_requestId && m_streamed) {
            out() << Qt::endl;
            err() << tr("(the answer was not in the right language: %1; writing it again)").arg(reason)
                  << Qt::endl;
        }
    });
    connect(&m_client, &DaemonClient::done, &loop, [this](const QString& id, const QString& result) {
        if (id == m_requestId) {
            printResult(result);
            m_loop->quit();
        }
    });
    connect(&m_client, &DaemonClient::error, &loop,
            [this](const QString& id, const QString& code, const QString& message) {
                if (id == m_requestId) {
                    printError(code, message);
                    m_loop->quit();
                }
            });

    const QDBusMessage reply = m_client.call(u"RunSkill"_s, {skillId, input, options()});
    if (reply.type() != QDBusMessage::ReplyMessage) {
        return replyError(reply);
    }
    m_requestId = reply.arguments().value(0).toString();
    loop.exec();
    m_loop = nullptr;
    return m_code;
}

void Cli::printResult(const QString& result) const
{
    const QJsonObject r = parseJson(result).toObject();
    if (m_command.json) {
        out() << result << Qt::endl;
    } else if (r.contains(u"replies"_s)) {
        for (const auto& reply : r.value(u"replies"_s).toArray()) {
            out() << reply.toString() << '\n';
        }
        out() << Qt::flush;
    } else if (m_stream) {
        out() << Qt::endl; // the answer is on screen already
    } else {
        out() << r.value(u"output"_s).toString().trimmed() << Qt::endl;
    }
    if (const QString warning = r.value(u"warning"_s).toString(); !warning.isEmpty()) {
        err() << tr("warning: %1").arg(warning) << Qt::endl;
    }
}

void Cli::printError(const QString& code, const QString& message)
{
    if (m_streamed) {
        out() << Qt::endl;
    }
    m_code = code == u"cancelled"_s && m_interrupted ? kExitInterrupted : kExitFailed;
    if (m_code != kExitInterrupted) {
        err() << message << Qt::endl;
    }
}

QVariantMap Cli::options() const
{
    QVariantMap o;
    if (!m_command.model.isEmpty()) {
        o.insert(u"model"_s, m_command.model);
    }
    return o;
}

QJsonObject Cli::runOne(const QString& skillId, const QVariantMap& input)
{
    // Timed as the client sees it, D-Bus included: from the call to the first piece of text
    // and to the answer.
    QElapsedTimer clock;
    QString requestId;
    double firstToken = 0.0;
    QJsonObject result;
    QEventLoop loop;
    connect(&m_client, &DaemonClient::token, &loop, [&](const QString& id, int, const QString&) {
        if (id == requestId && firstToken == 0.0) {
            firstToken = static_cast<double>(clock.nsecsElapsed()) / 1e9;
        }
    });
    connect(&m_client, &DaemonClient::done, &loop, [&](const QString& id, const QString& done) {
        if (id == requestId) {
            result = parseJson(done).toObject();
            loop.quit();
        }
    });
    connect(&m_client, &DaemonClient::error, &loop,
            [&](const QString& id, const QString& code, const QString& message) {
                if (id == requestId) {
                    result = {{u"error"_s, code}, {u"message"_s, message}};
                    loop.quit();
                }
            });
    clock.start();
    const QDBusMessage reply = m_client.call(u"RunSkill"_s, {skillId, input, options()});
    if (reply.type() != QDBusMessage::ReplyMessage) {
        return {{u"error"_s, reply.errorName().section(u'.', -1)}, {u"message"_s, reply.errorMessage()}};
    }
    requestId = reply.arguments().value(0).toString();
    loop.exec();
    result.insert(u"client_ttft_s"_s, firstToken);
    result.insert(u"client_total_s"_s, static_cast<double>(clock.nsecsElapsed()) / 1e9);
    return result;
}

int Cli::batch()
{
    // One request per line of standard input, {"skill": ..., "input": {...}}; one result per
    // line of standard output, in order: the Done JSON with the client's timings, or
    // {"error": ..., "message": ...}. For the eval harness and the latency bench.
    QFile in;
    if (!in.open(stdin, QIODevice::ReadOnly)) {
        return kExitFailed;
    }
    int number = 0;
    while (!in.atEnd()) {
        const QByteArray line = in.readLine().trimmed();
        ++number;
        if (line.isEmpty()) {
            continue;
        }
        QJsonParseError parseError{};
        const QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);
        QJsonObject result;
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            const QString why = parseError.error != QJsonParseError::NoError
                                    ? parseError.errorString()
                                    : tr(R"(expected an object like {"skill": ..., "input": ...})");
            result = {{u"error"_s, u"InvalidJson"_s}, {u"message"_s, tr("line %1: %2").arg(number).arg(why)}};
        } else {
            const QJsonObject request = doc.object();
            const QString skill = request.value(u"skill"_s).toString();
            const QJsonValue given = request.value(u"input"_s);
            // "input" is an object of inputs, or a plain string: the text the skill works on.
            const QVariantMap input = given.isString() ? QVariantMap{{mainInput(skill), given.toString()}}
                                                       : given.toObject().toVariantMap();
            result = runOne(skill, input);
        }
        result.insert(u"line"_s, number);
        out() << QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact)) << Qt::endl;
    }
    return kExitOk;
}

void Cli::interrupt()
{
    m_interrupted = true;
    if (!m_requestId.isEmpty()) {
        (void)m_client.call(u"Cancel"_s, {m_requestId});
    }
}

QString Cli::mainInput(const QString& skillId)
{
    if (m_mainInputs.isEmpty()) {
        const QDBusMessage reply = m_client.call(u"ListSkills"_s, {});
        if (reply.type() == QDBusMessage::ReplyMessage) {
            for (const auto& s : parseJson(reply.arguments().value(0).toString()).toArray()) {
                const QJsonObject o = s.toObject();
                m_mainInputs.insert(o.value(u"id"_s).toString(),
                                    o.value(u"main_input"_s).toString(u"text"_s));
            }
        }
    }
    return m_mainInputs.value(skillId, u"text"_s);
}

int Cli::listSkills()
{
    const QDBusMessage reply = m_client.call(u"ListSkills"_s, {});
    if (reply.type() != QDBusMessage::ReplyMessage) {
        return replyError(reply);
    }
    for (const auto& s : parseJson(reply.arguments().value(0).toString()).toArray()) {
        const QJsonObject o = s.toObject();
        out() << o.value(u"id"_s).toString().leftJustified(24) << o.value(u"title"_s).toString() << '\n';
    }
    out() << Qt::flush;
    return kExitOk;
}

int Cli::status()
{
    const QDBusMessage reply = m_client.call(u"Status"_s, {});
    if (reply.type() != QDBusMessage::ReplyMessage) {
        return replyError(reply);
    }
    out() << QJsonDocument(parseJson(reply.arguments().value(0).toString()).toObject()).toJson();
    out() << Qt::flush;
    return kExitOk;
}

QString Cli::resolveModel(const QString& name)
{
    // "fast" and "quality" name the tier's model.
    const QDBusMessage reply = m_client.call(u"ListModels"_s, {});
    for (const auto& m : parseJson(reply.arguments().value(0).toString()).toArray()) {
        const QJsonObject o = m.toObject();
        if (o.value(u"id"_s).toString() == name || o.value(u"tiers"_s).toArray().contains(name)) {
            return o.value(u"id"_s).toString();
        }
    }
    return name;
}

int Cli::models()
{
    const QString action = m_command.args.value(0, u"list"_s);
    const QString target = m_command.args.value(1);
    if (action == u"list"_s) {
        const QDBusMessage reply = m_client.call(u"ListModels"_s, {});
        if (reply.type() != QDBusMessage::ReplyMessage) {
            return replyError(reply);
        }
        for (const auto& m : parseJson(reply.arguments().value(0).toString()).toArray()) {
            const QJsonObject o = m.toObject();
            QString state = o.value(u"installed"_s).toBool() ? tr("installed") : tr("not installed");
            if (!o.value(u"fits"_s).toBool()) {
                state += tr(", needs %1 GB memory").arg(o.value(u"ram_required_gb"_s).toDouble());
            }
            const QStringList tiers = o.value(u"tiers"_s).toVariant().toStringList();
            out() << o.value(u"id"_s).toString().leftJustified(24)
                  << (tiers.isEmpty() ? o.value(u"role"_s).toString() : tiers.join(u',')).leftJustified(9)
                  << size(o.value(u"size"_s).toInteger()).leftJustified(10) << state << '\n';
        }
        out() << Qt::flush;
        return kExitOk;
    }
    if (target.isEmpty() || (action != u"download"_s && action != u"pause"_s && action != u"delete"_s)) {
        err() << tr("Usage: paraphrase models [list | download MODEL | pause MODEL | delete MODEL]; MODEL is "
                    "fast, quality or an id from the list.")
              << Qt::endl;
        return kExitUsage;
    }
    const QString id = resolveModel(target);
    if (action == u"download"_s) {
        return download(id);
    }
    const QDBusMessage reply =
        m_client.call(action == u"pause"_s ? u"PauseDownload"_s : u"DeleteModel"_s, {id});
    return reply.type() == QDBusMessage::ReplyMessage ? kExitOk : replyError(reply);
}

int Cli::download(const QString& modelId)
{
    int code = kExitOk;
    QEventLoop loop;
    const bool terminal = stdoutIsTerminal();
    connect(&m_client, &DaemonClient::downloadProgress, &loop, [&](const QString& status) {
        const QJsonObject s = parseJson(status).toObject();
        if (s.value(u"model"_s).toString() != modelId) {
            return;
        }
        const QString state = s.value(u"state"_s).toString();
        if (terminal && state == u"downloading"_s) {
            out() << '\r'
                  << tr("%1: %2 of %3")
                         .arg(modelId, size(s.value(u"received"_s).toInteger()),
                              size(s.value(u"total"_s).toInteger()))
                  << Qt::flush;
        } else if (state == u"verifying"_s) {
            out() << (terminal ? u"\r"_s : QString())
                  << tr("%1: checking its SHA-256 fingerprint").arg(modelId) << Qt::endl;
        } else if (state == u"installed"_s) {
            out() << tr("%1: installed").arg(modelId) << Qt::endl;
            loop.quit();
        } else if (state == u"failed"_s || state == u"paused"_s) {
            err() << modelId << ": " << (state == u"paused"_s ? tr("paused") : s.value(u"error"_s).toString())
                  << Qt::endl;
            code = kExitFailed;
            loop.quit();
        }
    });
    const QDBusMessage reply = m_client.call(u"DownloadModel"_s, {modelId});
    if (reply.type() != QDBusMessage::ReplyMessage) {
        return replyError(reply);
    }
    // Already installed: the daemon says so at once, or there is nothing to wait for.
    const QDBusMessage list = m_client.call(u"ListModels"_s, {});
    for (const auto& m : parseJson(list.arguments().value(0).toString()).toArray()) {
        const QJsonObject o = m.toObject();
        if (o.value(u"id"_s).toString() == modelId && o.value(u"installed"_s).toBool() &&
            !o.contains(u"download"_s)) {
            out() << tr("%1: installed").arg(modelId) << Qt::endl;
            return kExitOk;
        }
    }
    loop.exec();
    return code;
}

} // namespace paraphrase::cli

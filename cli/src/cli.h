#pragma once

#include "daemon_client.h"

#include <QEventLoop>
#include <QObject>
#include <QStringList>
#include <QVariantMap>

namespace paraphrase::cli {

/// What a command-line run asks for, parsed from the arguments.
struct Command
{
    QString name;          ///< proofread, rewrite, reply, run, skills, status, models
    QStringList args;      ///< the positional arguments after the command
    QString tone;          ///< rewrite: a preset of the rewrite skill
    QString to;            ///< target_lang
    QString draft;         ///< reply: what the user started typing
    QString context;       ///< run: the context input
    bool json = false;     ///< print the Done JSON
    bool noStream = false; ///< print the answer only when it is complete
};

/// The `paraphrase` command (PLAN 4, DOCS/api.md): runs skills through the daemon and prints
/// the answer, so it serves scripts and the hotkey fallback. Text comes from the arguments or
/// standard input; the answer goes to standard output, everything else to standard error.
class Cli : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Cli)

public:
    static constexpr int kExitOk = 0;
    static constexpr int kExitFailed = 1;
    static constexpr int kExitUsage = 2;
    static constexpr int kExitNoModel = 3;
    static constexpr int kExitInterrupted = 130;

    explicit Cli(Command command, QObject* parent = nullptr);
    ~Cli() override = default;

    /// Runs the command; the process's exit code.
    [[nodiscard]] int exec();
    /// Ctrl+C: cancels the running request.
    void interrupt();

private:
    [[nodiscard]] int runSkill(const QString& skillId, const QVariantMap& input);
    [[nodiscard]] int listSkills();
    [[nodiscard]] int status();
    [[nodiscard]] int models();
    [[nodiscard]] int download(const QString& modelId);
    [[nodiscard]] static int replyError(const QDBusMessage& reply);
    void printResult(const QString& result) const;
    void printError(const QString& code, const QString& message);
    [[nodiscard]] QString inputText() const;
    [[nodiscard]] QString resolveModel(const QString& name);

    Command m_command;
    DaemonClient m_client;
    QString m_requestId;
    bool m_interrupted = false;
    bool m_stream = false;   ///< the answer streams to a terminal
    bool m_streamed = false; ///< something was printed already
    int m_code = kExitOk;
    QEventLoop* m_loop = nullptr; ///< the running request's loop
};

} // namespace paraphrase::cli

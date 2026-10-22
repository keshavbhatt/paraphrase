// The ranked backends (PLAN 6): the first available one of each kind is used, the clipboard
// and "bind this command" always work, and the status lists every backend for Settings.

#include "integration/clipboard_backends.h"
#include "integration/integrations.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QTest>

using namespace Qt::StringLiterals;
using namespace paraphrase::app;

namespace {

class FakeCapture final : public CaptureBackend
{
public:
    explicit FakeCapture(bool works)
        : m_works(works)
    {}
    [[nodiscard]] QString name() const override { return u"fake capture"_s; }
    [[nodiscard]] bool available() const override { return m_works; }
    [[nodiscard]] std::optional<CapturedText> capture() override
    {
        return CapturedText{.text = u"from fake"_s, .origin = u"selection"_s, .appName = {}, .window = {}};
    }

private:
    bool m_works;
};

class FakeShortcut final : public ShortcutBackend
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(FakeShortcut)

public:
    FakeShortcut(bool works, QObject* parent)
        : ShortcutBackend(parent)
        , m_works(works)
    {}
    ~FakeShortcut() override = default;
    [[nodiscard]] QString name() const override { return u"fake shortcut"_s; }
    [[nodiscard]] bool available() const override { return m_works; }
    bool bind(const QKeySequence& /*keys*/) override { return m_works; }
    [[nodiscard]] QString description() const override { return u"Ctrl+Alt+Space"_s; }

private:
    bool m_works;
};

} // namespace

class TestIntegrations : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void usesTheFirstBackendThatWorks()
    {
        std::vector<std::unique_ptr<CaptureBackend>> capture;
        capture.push_back(std::make_unique<FakeCapture>(false));
        capture.push_back(std::make_unique<FakeCapture>(true));
        capture.push_back(std::make_unique<ClipboardCapture>());
        std::vector<std::unique_ptr<InsertBackend>> insert;
        insert.push_back(std::make_unique<ClipboardInsert>());
        QObject owner;
        const Integrations i(std::move(capture), std::move(insert),
                             {new FakeShortcut(true, &owner), new CommandShortcut(&owner)});
        QCOMPARE(i.capture().capture()->text, u"from fake"_s);
        QVERIFY(i.hasGlobalShortcut());
        QCOMPARE(i.status().size(), 6);
    }

    void fallsBackToTheClipboardAndACommand()
    {
        QObject owner;
        const Integrations i = Integrations::forThisSession(&owner);
        QVERIFY(!i.hasGlobalShortcut());
        QVERIFY(i.shortcut().description().contains(u"paraphrase-app --popup"_s));
        QGuiApplication::clipboard()->setText(u"copied text"_s);
        const auto got = i.capture().capture();
        QVERIFY(got.has_value());
        QVERIFY(!got->text.isEmpty());
        // Inserting never types here: it leaves the result on the clipboard.
        QCOMPARE(i.insert().insert(u"the result"_s, *got), InsertOutcome::OnClipboard);
        QCOMPARE(QGuiApplication::clipboard()->text(), u"the result"_s);
    }
};

QTEST_MAIN(TestIntegrations)
#include "tst_integrations.moc"

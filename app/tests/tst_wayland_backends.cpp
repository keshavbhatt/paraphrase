// Wayland data-control against a headless KWin started by scripts/desktop-test.sh, never the
// user's session. "Another app" is a second client on its own thread, as a separate app
// would be: it owns the clipboard or reads ours, while this thread does the same.

#include "integration/data_control_client.h"
#include "integration/selection_captures.h"

#include <QGuiApplication>
#include <QMutex>
#include <QTest>
#include <QThread>

#include <functional>

using namespace Qt::StringLiterals;
using namespace paraphrase::app;

namespace {

/// A data-control client living on its own thread with its own event loop.
class OtherApp : public QThread
{
public:
    OtherApp()
    {
        start();
        while (m_ready.loadAcquire() == 0) {
            QThread::msleep(10);
        }
    }
    ~OtherApp() override
    {
        quit();
        wait();
    }
    OtherApp(const OtherApp&) = delete;
    OtherApp& operator=(const OtherApp&) = delete;
    OtherApp(OtherApp&&) = delete;
    OtherApp& operator=(OtherApp&&) = delete;

    [[nodiscard]] bool connected() const { return m_client != nullptr; }

    /// Runs `work` on the other thread and waits; only for calls that need nothing from us.
    void call(const std::function<void(DataControlClient&)>& work)
    {
        QMetaObject::invokeMethod(
            m_client.get(), [this, work] { work(*m_client); }, Qt::BlockingQueuedConnection);
    }
    /// Runs `work` on the other thread without waiting: for reads we must serve meanwhile.
    void post(const std::function<void(DataControlClient&)>& work)
    {
        QMetaObject::invokeMethod(m_client.get(), [this, work] { work(*m_client); }, Qt::QueuedConnection);
    }

protected:
    void run() override
    {
        m_client = DataControlClient::connectToCompositor();
        m_ready.storeRelease(1);
        exec();
        m_client.reset();
    }

private:
    std::unique_ptr<DataControlClient> m_client;
    QAtomicInt m_ready;
};

} // namespace

class TestWaylandBackends : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<OtherApp> m_other;
    std::shared_ptr<DataControlClient> m_ours;

private Q_SLOTS:
    void initTestCase()
    {
        if (qEnvironmentVariable("PARAPHRASE_TEST_NESTED") != u"1"_s) {
            QSKIP("run through scripts/desktop-test.sh: it starts a headless KWin for this test");
        }
        m_ours = DataControlClient::connectToCompositor();
        m_other = std::make_unique<OtherApp>();
        QVERIFY2(m_ours && m_other->connected(), "the compositor offers no ext-data-control-v1");
    }

    void readsWhatAnotherAppCopied()
    {
        m_other->call(
            [](DataControlClient& c) { c.setClipboard({{u"text/plain;charset=utf-8"_s, "Mia dont know"}}); });
        QTRY_COMPARE_WITH_TIMEOUT(m_ours->readText(DataControlClient::Which::Clipboard).value_or(QString()),
                                  u"Mia dont know"_s, 3000);
    }

    void anotherAppReadsWhatWeSet()
    {
        DataControlClipboard ours(m_ours);
        ours.setText(u"the result"_s);
        QMutex lock;
        QString seen;
        // The other app reads while this thread serves the data.
        QTRY_VERIFY_WITH_TIMEOUT(
            [&] {
                m_other->post([&](DataControlClient& c) {
                    const QString text = c.readText(DataControlClient::Which::Clipboard).value_or(QString());
                    const QMutexLocker guard(&lock);
                    seen = text;
                });
                QTest::qWait(200);
                const QMutexLocker guard(&lock);
                return seen == u"the result"_s;
            }(),
            5000);
    }

    void savesAndRestoresEveryFormat()
    {
        m_other->call([](DataControlClient& c) {
            c.setClipboard({{u"text/plain"_s, "copied earlier"}, {u"text/html"_s, "<b>copied</b> earlier"}});
        });
        DataControlClipboard ours(m_ours);
        QTRY_VERIFY_WITH_TIMEOUT(
            m_ours->mimeTypes(DataControlClient::Which::Clipboard).contains(u"text/html"_s), 3000);
        const ClipboardContent saved = ours.save(); // before a paste
        QCOMPARE(saved.size(), 2);
        ours.setText(u"the result"_s);
        QVERIFY(!m_ours->mimeTypes(DataControlClient::Which::Clipboard).contains(u"text/html"_s));
        ours.restore(saved); // after it
        QCOMPARE(m_ours->read(DataControlClient::Which::Clipboard, u"text/html"_s),
                 QByteArray("<b>copied</b> earlier"));
        // The other app sees the restored formats too.
        QStringList theirs;
        QTRY_VERIFY_WITH_TIMEOUT(
            [&] {
                m_other->call(
                    [&](DataControlClient& c) { theirs = c.mimeTypes(DataControlClient::Which::Clipboard); });
                return theirs.contains(u"text/html"_s);
            }(),
            3000);
    }

    void capturesTheClipboardWhenNothingIsHighlighted()
    {
        m_other->call([](DataControlClient& c) { c.setClipboard({{u"text/plain"_s, "copied text"}}); });
        DataControlCapture capture(m_ours, nullptr);
        const auto next = [&capture] { return capture.capture().value_or(CapturedText{}); };
        QTRY_COMPARE_WITH_TIMEOUT(next().text, u"copied text"_s, 3000);
        const CapturedText got = next();
        QCOMPARE(got.origin, u"clipboard"_s);
        QVERIFY(got.window.isEmpty()); // no window tracker: nothing would be typed
    }
};

QTEST_GUILESS_MAIN(TestWaylandBackends)
#include "tst_wayland_backends.moc"

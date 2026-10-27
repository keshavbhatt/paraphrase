// The README's screenshots: real actions with the real models, each window grabbed in the dark
// theme for scripts/screenshots.py to frame. By hand only, through scripts/screenshots.sh, which
// runs it on a private session bus with a scratch profile. Never part of ctest's default run.

#include "daemon_client.h"
#include "integration/clipboard_backends.h"
#include "integration/integrations.h"
#include "popup/popup_controller.h"
#include "popup/popup_window.h"
#include "ui/common/chip_button.h"
#include "ui/main/main_window.h"
#include "ui/settings/settings_window.h"

#include <QAbstractButton>
#include <QDir>
#include <QMenu>
#include <QSignalSpy>
#include <QTest>
#include <QTextEdit>

using namespace Qt::StringLiterals;
using paraphrase::app::MainWindow;

namespace {

constexpr int kTimeoutMs = 300'000; // the quality model on a busy computer
constexpr QSize kWindow{1180, 860};

} // namespace

class TestScreenshots : public QObject
{
    Q_OBJECT

private:
    QString m_dir;

    static void waitFor(MainWindow& w, const QSignalSpy& finished)
    {
        QVERIFY(QTest::qWaitFor([&] { return finished.size() == 1; }, kTimeoutMs));
        QTest::qWait(300); // the last repaint after the answer
        Q_UNUSED(w);
    }

    void save(QWidget* w, const QString& name) const
    {
        QCoreApplication::processEvents();
        QVERIFY(w->grab().save(m_dir + u"/"_s + name + u".png"_s));
    }

    static void run(MainWindow& w, const QString& skill, const QString& text)
    {
        w.selectSkill(skill);
        QCOMPARE(w.currentSkill(), skill);
        w.findChild<QTextEdit*>(u"text"_s)->setPlainText(text);
        const QSignalSpy finished(&w, &MainWindow::finished);
        w.run();
        waitFor(w, finished);
    }

private Q_SLOTS:
    void initTestCase()
    {
        m_dir = qEnvironmentVariable("PARAPHRASE_SHOTS_DIR");
        if (m_dir.isEmpty()) {
            QSKIP("run scripts/screenshots.sh: it sets PARAPHRASE_SHOTS_DIR and a scratch profile");
        }
        QDir().mkpath(m_dir);
    }

    void theMainWindow()
    {
        MainWindow w;
        w.setGlobalShortcut(true); // as on a desktop that has one: no notice about it
        w.resize(kWindow);
        w.show();
        w.start();

        run(w, u"proofread"_s,
            u"Hi Sam,\n\nThank you for sending the report so quickly. I have went through it and "
            "their are only two small things to fix before friday.\n\nThe sales figures for March "
            "is missing, and the chart on page 4 have the wrong title.\n\nBest regards,\nPriya"_s);
        save(&w, u"proofread"_s);

        run(w, u"rewrite.professional"_s,
            u"hey, cant make it to the call tomorrow, something came up. can we do thursday "
            "instead? also still waiting on those numbers from you"_s);
        save(&w, u"tone"_s);

        run(w, u"summarize"_s,
            u"The vendor confirmed that the shipment left Mumbai on 3 March and should reach Pune "
            "by 7 March. Two cartons were damaged in transit; the vendor will replace them at no "
            "cost and send them with the next delivery. The invoice of Rs 48,500 is due on 15 "
            "March. Please confirm the delivery address and a contact person at the warehouse by "
            "Thursday, so the driver can call ahead. The vendor also asked whether we want the "
            "April order brought forward by a week, as their factory closes for maintenance from "
            "22 April."_s);
        save(&w, u"summarize"_s);

        // Translate: English into Hindi, from the action's language menu.
        w.selectSkill(u"proofread"_s);
        w.findChild<QTextEdit*>(u"text"_s)->setPlainText(
            u"The pharmacy on Station Road is open until 9 pm today. Please pick up "
            "the medicine for your mother on the way home, and keep the receipt."_s);
        const auto* translate = w.findChild<paraphrase::app::ChipButton*>(u"action-translate"_s);
        QVERIFY(translate != nullptr);
        Q_EMIT translate->menu()->aboutToShow();
        auto* hindi = translate->menu()->findChild<QAction*>(u"lang-hi"_s);
        QVERIFY(hindi != nullptr);
        {
            const QSignalSpy finished(&w, &MainWindow::finished);
            hindi->trigger();
            waitFor(w, finished);
        }
        save(&w, u"translate"_s);
    }

    void aFullReply()
    {
        // A window of its own: once something ran, switching to Full reply runs again at once,
        // on the text the other actions left.
        MainWindow w;
        w.setGlobalShortcut(true);
        w.resize(kWindow);
        w.show();
        w.start();
        // A full reply: the message, what the user wants to say, an email.
        w.selectSkill(u"reply"_s);
        auto* full = w.findChild<QAbstractButton*>(u"fullReply"_s);
        QVERIFY(full != nullptr);
        full->click();
        w.findChild<QTextEdit*>(u"fullMessage"_s)
            ->setPlainText(u"Hi Priya,\n\nCould we get on a call this Thursday at 3 pm to go through "
                           "the delivery delays? I would also like the updated cost sheet before "
                           "then.\n\nBest,\nDaniel"_s);
        w.findChild<QTextEdit*>(u"intent"_s)
            ->setPlainText(u"thursday doesnt work, friday 11 am ok? cost sheet by wednesday"_s);
        {
            const QSignalSpy finished(&w, &MainWindow::finished);
            w.run();
            waitFor(w, finished);
        }
        save(&w, u"reply"_s);
    }

    void thePopup()
    {
        QObject owner;
        std::vector<std::unique_ptr<paraphrase::app::CaptureBackend>> capture;
        capture.push_back(std::make_unique<paraphrase::app::ClipboardCapture>());
        std::vector<std::unique_ptr<paraphrase::app::InsertBackend>> insert;
        insert.push_back(std::make_unique<paraphrase::app::ClipboardInsert>());
        const paraphrase::app::Integrations backends(std::move(capture), std::move(insert),
                                                     {new paraphrase::app::CommandShortcut(&owner)});
        paraphrase::app::PopupController popup(backends);
        popup.showText({
            .text = u"Mia dont know where the keys are, can you check the drawer next to the fridge"_s,
            .origin = u"selection"_s,
            .appName = u"Telegram"_s,
            .window = {},
        });
        popup.window()->show();
        const QSignalSpy finished(&popup, &paraphrase::app::PopupController::finished);
        popup.window()->findChild<paraphrase::app::ChipButton*>(u"action-proofread"_s)->click();
        QVERIFY(QTest::qWaitFor([&] { return finished.size() == 1; }, kTimeoutMs));
        QTest::qWait(300);
        save(popup.window(), u"popup"_s);
    }

    void theModels()
    {
        paraphrase::client::DaemonClient client;
        QVERIFY(client.connectToDaemon().isEmpty());
        paraphrase::app::SettingsWindow w(client);
        w.resize(kWindow);
        w.show();
        w.showPage(paraphrase::app::SettingsWindow::Models);
        QTest::qWait(500); // the list comes from the daemon
        save(&w, u"models"_s);
    }
};

QTEST_MAIN(TestScreenshots)
#include "tst_screenshots.moc"

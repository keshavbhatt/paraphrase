// Every action through the window and the popup with the real models on this computer, saving
// a screenshot of each result: the check that fake answers cannot give (long outputs, real
// JSON, Hindi and Hinglish). By hand only: PARAPHRASE_TEST_REAL=1 PARAPHRASE_SCREENSHOTS=<dir>,
// with PARAPHRASE_MODELS_DIR pointing at installed models. Never part of ctest's default run.

#include "integration/clipboard_backends.h"
#include "integration/integrations.h"
#include "popup/popup_controller.h"
#include "popup/popup_window.h"
#include "ui/common/chip_button.h"
#include "ui/main/main_window.h"

#include <QDir>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <QTextEdit>

using namespace Qt::StringLiterals;
using paraphrase::app::MainWindow;

namespace {

constexpr int kTimeoutMs = 180'000; // the quality model on a busy CPU

struct Case
{
    QString name;
    QString skill;
    QString text;
};

} // namespace

class TestRealModels : public QObject
{
    Q_OBJECT

private:
    QString m_dir;

    static void runAndWait(MainWindow& w)
    {
        const QSignalSpy finished(&w, &MainWindow::finished);
        w.run();
        QVERIFY(QTest::qWaitFor([&] { return finished.size() == 1; }, kTimeoutMs));
    }

private Q_SLOTS:
    void initTestCase()
    {
        if (qEnvironmentVariable("PARAPHRASE_TEST_REAL") != u"1"_s) {
            QSKIP("set PARAPHRASE_TEST_REAL=1 (and PARAPHRASE_MODELS_DIR) to run with the real models");
        }
        m_dir = qEnvironmentVariable("PARAPHRASE_SCREENSHOTS", QDir::tempPath() + u"/paraphrase-real"_s);
        QDir().mkpath(m_dir);
    }

    void everyActionInTheWindow()
    {
        const QList<Case> cases = {
            {
                .name = u"proofread-en"_s,
                .skill = u"proofread"_s,
                .text =
                    u"Their going to the market tomorow, dont forget the list.\nCan you call Priya at 6pm"_s,
            },
            {
                .name = u"proofread-hi"_s,
                .skill = u"proofread"_s,
                .text = u"मेरी भाई कल दिल्ली जाएगी और वो वहां दो दिन रहेगा।"_s,
            },
            {
                .name = u"tone-polite"_s,
                .skill = u"rewrite.polite"_s,
                .text = u"send me the Q3 report by friday, its already late"_s,
            },
            {
                .name = u"tone-hinglish"_s,
                .skill = u"rewrite.professional"_s,
                .text = u"bhai kal office nahi aa paunga, tabiyat thik nahi hai"_s,
            },
            {
                .name = u"summarize"_s,
                .skill = u"summarize"_s,
                .text =
                    u"The vendor confirmed the shipment left Mumbai on March 3 and should reach Pune by "
                    u"March 7. "
                    "Two cartons were damaged in transit; they will be replaced at no cost. The invoice of "
                    "Rs 48,500 is due on March 15. Please confirm the delivery address and a contact person "
                    "by Thursday."_s,
            },
            {
                .name = u"format-bullets"_s,
                .skill = u"format.bullets"_s,
                .text =
                    u"we need milk eggs and bread, also call the plumber about the kitchen tap and pay the "
                    "electricity bill before the 10th"_s,
            },
        };
        MainWindow w;
        w.resize(1180, 760);
        w.show();
        w.start();
        for (const Case& c : cases) {
            w.selectSkill(c.skill);
            if (w.currentSkill() != c.skill) {
                qWarning("skipped %s: no skill %s", qPrintable(c.name), qPrintable(c.skill));
                continue;
            }
            w.findChild<QTextEdit*>(u"text"_s)->setPlainText(c.text);
            runAndWait(w);
            QVERIFY2(!w.resultText().trimmed().isEmpty(), qPrintable(c.name));
            QVERIFY(w.grab().save(m_dir + u"/real-window-"_s + c.name + u".png"_s));
        }
    }

    void repliesInTheWindow()
    {
        MainWindow w;
        w.resize(1180, 760);
        w.show();
        w.start();
        w.selectSkill(u"reply"_s);
        w.findChild<QTextEdit*>(u"message"_s)
            ->setPlainText(u"bhaiya parcel aapke ghar ke bahar rakh du? koi phone nahi utha raha"_s);
        runAndWait(w);
        QVERIFY(!w.resultText().isEmpty());
        QVERIFY(w.grab().save(m_dir + u"/real-window-reply.png"_s));
        auto* full = w.findChild<QPushButton*>(u"fullReply"_s);
        if (full == nullptr || full->parentWidget()->isHidden()) {
            return;
        }
        full->click();
        w.findChild<QTextEdit*>(u"fullMessage"_s)
            ->setPlainText(
                u"Hi Priya, could we get on a call this Thursday at 3 pm to go through the delivery "
                "delays? I'd also like the updated cost sheet before then. Best, Daniel"_s);
        w.findChild<QTextEdit*>(u"intent"_s)
            ->setPlainText(u"Thursday doesn't work, offer Friday 11 am. Cost sheet by Wednesday."_s);
        runAndWait(w);
        QVERIFY(w.resultText().contains(u"11"_s));
        QVERIFY(w.grab().save(m_dir + u"/real-window-full-reply.png"_s));
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
        const paraphrase::app::CapturedText text{
            .text = u"Mia dont know where the keys are, can you check the drawer next to the fridge"_s,
            .origin = u"selection"_s,
            .appName = u"Telegram"_s,
            .window = {},
        };
        popup.showText(text);
        popup.window()->show();
        const QSignalSpy finished(&popup, &paraphrase::app::PopupController::finished);
        popup.window()->findChild<paraphrase::app::ChipButton*>(u"action-proofread"_s)->click();
        QVERIFY(QTest::qWaitFor([&] { return finished.size() == 1; }, kTimeoutMs));
        QVERIFY(popup.window()->grab().save(m_dir + u"/real-popup-proofread.png"_s));
    }
};

QTEST_MAIN(TestRealModels)
#include "tst_real_models.moc"

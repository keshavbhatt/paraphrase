// The build links llama.cpp and its CPU backend initialises.

#include <QTest>

#include <llama.h>

class TestSmoke : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void backendInitialises()
    {
        llama_backend_init();
        const QByteArray info(llama_print_system_info());
        QVERIFY2(info.contains("CPU"), info.constData());
        QVERIFY(llama_max_devices() >= 1);
        llama_backend_free();
    }
};

QTEST_GUILESS_MAIN(TestSmoke)
#include "tst_smoke.moc"

#include "PythonClient.h"

#include <qtpyt/globalinit.h>

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class PythonClientTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qtpyt::init();
    }

    void loadMissingScriptEmitsError()
    {
        PythonClient client;
        QSignalSpy errors(&client, &PythonClient::errorOccurred);

        QVERIFY(!client.loadScript("/tmp/redisc-missing-python-client-test.py"));
        QVERIFY(!client.isLoaded());
        QCOMPARE(errors.count(), 1);
        QVERIFY(errors.takeFirst().at(0).toString().contains("does not exist"));
    }

    void messageCallbackCanUseClientApi()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        const QString scriptPath = dir.filePath("processor.py");
        QFile script(scriptPath);
        QVERIFY(script.open(QIODevice::WriteOnly | QIODevice::Text));
        script.write(R"PY(
def on_loaded():
    log("loaded")

def on_redis_message(channel, message):
    log(channel + ":" + message)
    publish(channel + ".out", message.upper())
    subscribe(channel + ".extra")
    unsubscribe(channel + ".old")
)PY");
        script.close();

        PythonClient client;
        QSignalSpy published(&client, &PythonClient::publishRequested);
        QSignalSpy subscribed(&client, &PythonClient::subscribeRequested);
        QSignalSpy unsubscribed(&client, &PythonClient::unsubscribeRequested);
        QSignalSpy logs(&client, &PythonClient::logMessage);
        QSignalSpy errors(&client, &PythonClient::errorOccurred);

        QVERIFY(client.loadScript(scriptPath));
        QVERIFY(client.isLoaded());
        QCOMPARE(client.scriptPath(), scriptPath);
        QVERIFY(errors.isEmpty());
        QVERIFY(logs.count() >= 2);
        QCOMPARE(logs.at(0).at(0).toString(), QString("loaded"));

        client.notifyRedisMessage("orders", "created");

        QCOMPARE(published.count(), 1);
        QCOMPARE(published.at(0).at(0).toString(), QString("orders.out"));
        QCOMPARE(published.at(0).at(1).toString(), QString("CREATED"));

        QCOMPARE(subscribed.count(), 1);
        QCOMPARE(subscribed.at(0).at(0).toString(), QString("orders.extra"));

        QCOMPARE(unsubscribed.count(), 1);
        QCOMPARE(unsubscribed.at(0).at(0).toString(), QString("orders.old"));

        QCOMPARE(logs.last().at(0).toString(), QString("orders:created"));
        QVERIFY(errors.isEmpty());
    }
};

QTEST_GUILESS_MAIN(PythonClientTest)

#include "PythonClientTest.moc"

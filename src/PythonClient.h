#pragma once

#include <QObject>
#include <QString>
#include <memory>

namespace qtpyt {
class QPyModule;
}

class PythonClient : public QObject {
    Q_OBJECT

public:
    explicit PythonClient(QObject *parent = nullptr);
    ~PythonClient() override;

    bool loadScript(const QString &path);
    void clear();
    bool isLoaded() const;
    QString scriptPath() const;
    void notifyRedisMessage(const QString &channel, const QString &message);

signals:
    void publishRequested(const QString &channel, const QString &message);
    void subscribeRequested(const QString &channel);
    void unsubscribeRequested(const QString &channel);
    void logMessage(const QString &message);
    void errorOccurred(const QString &message);

private:
    std::unique_ptr<qtpyt::QPyModule> m_module;
    QString m_scriptPath;
    bool m_warnedMissingHandler = false;
};

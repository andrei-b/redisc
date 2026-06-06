#include "PythonClient.h"

#include <qtpyt/qpymodule.h>

#include <QFileInfo>
#include <QVariant>

PythonClient::PythonClient(QObject *parent)
    : QObject(parent)
{
}

PythonClient::~PythonClient() = default;

bool PythonClient::loadScript(const QString &path)
{
    clear();
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty()) {
        emit errorOccurred("Choose a Python script before loading.");
        return false;
    }
    if (!QFileInfo::exists(trimmed)) {
        emit errorOccurred(QString("Python script does not exist: %1").arg(trimmed));
        return false;
    }

    try {
        m_module = std::make_unique<qtpyt::QPyModule>(trimmed, qtpyt::QPySourceType::File);
        m_scriptPath = trimmed;
        m_warnedMissingHandler = false;

        m_module->addFunction("publish", [this](const QVariantList &args) -> QVariant {
            if (args.size() >= 2) {
                emit publishRequested(args.at(0).toString(), args.at(1).toString());
            }
            return {};
        });
        m_module->addFunction("subscribe", [this](const QVariantList &args) -> QVariant {
            if (!args.isEmpty()) {
                emit subscribeRequested(args.at(0).toString());
            }
            return {};
        });
        m_module->addFunction("unsubscribe", [this](const QVariantList &args) -> QVariant {
            if (!args.isEmpty()) {
                emit unsubscribeRequested(args.at(0).toString());
            }
            return {};
        });
        m_module->addFunction("log", [this](const QVariantList &args) -> QVariant {
            if (!args.isEmpty()) {
                emit logMessage(args.at(0).toString());
            }
            return {};
        });

        try {
            m_module->call<void>("on_loaded");
        } catch (const std::exception &) {
            // Optional hook.
        }

        emit logMessage(QString("Loaded Python script: %1").arg(trimmed));
        return true;
    } catch (const std::exception &error) {
        clear();
        emit errorOccurred(QString("Python load failed: %1").arg(error.what()));
        return false;
    }
}

void PythonClient::clear()
{
    m_module.reset();
    m_scriptPath.clear();
    m_warnedMissingHandler = false;
}

bool PythonClient::isLoaded() const
{
    return m_module != nullptr;
}

QString PythonClient::scriptPath() const
{
    return m_scriptPath;
}

void PythonClient::notifyRedisMessage(const QString &channel, const QString &message)
{
    if (!m_module) {
        return;
    }

    try {
        m_module->call<void>("on_redis_message", channel, message);
    } catch (const std::exception &error) {
        const QString text = QString::fromLocal8Bit(error.what());
        if (text.contains("on_redis_message") && !m_warnedMissingHandler) {
            m_warnedMissingHandler = true;
            emit errorOccurred("Python script has no on_redis_message(channel, message) handler.");
        } else if (!text.contains("on_redis_message")) {
            emit errorOccurred(QString("Python message handler failed: %1").arg(text));
        }
    }
}

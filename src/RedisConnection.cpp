#include "RedisConnection.h"

#include <QDataStream>
#include <QTimer>

void RespParser::append(const QByteArray &data)
{
    m_buffer.append(data);
}

QList<RespValue> RespParser::takeValues()
{
    QList<RespValue> values;
    int offset = 0;
    while (offset < m_buffer.size()) {
        RespValue value;
        int next = offset;
        if (!parseOne(next, value)) {
            break;
        }
        values.append(value);
        offset = next;
    }
    if (offset > 0) {
        m_buffer.remove(0, offset);
    }
    return values;
}

int RespParser::lineEnd(int offset) const
{
    return m_buffer.indexOf("\r\n", offset);
}

bool RespParser::parseOne(int &offset, RespValue &value) const
{
    if (offset >= m_buffer.size()) {
        return false;
    }

    const char prefix = m_buffer.at(offset++);
    const int end = lineEnd(offset);
    if (end < 0) {
        return false;
    }
    const QByteArray line = m_buffer.mid(offset, end - offset);
    offset = end + 2;

    switch (prefix) {
    case '+':
        value.type = RespValue::Type::SimpleString;
        value.text = QString::fromUtf8(line);
        return true;
    case '-':
        value.type = RespValue::Type::Error;
        value.text = QString::fromUtf8(line);
        return true;
    case ':':
        value.type = RespValue::Type::Integer;
        value.integer = line.toLongLong();
        return true;
    case '$': {
        const int length = line.toInt();
        if (length < 0) {
            value.type = RespValue::Type::Null;
            return true;
        }
        if (offset + length + 2 > m_buffer.size()) {
            return false;
        }
        value.type = RespValue::Type::BulkString;
        value.text = QString::fromUtf8(m_buffer.mid(offset, length));
        offset += length + 2;
        return true;
    }
    case '*': {
        const int count = line.toInt();
        if (count < 0) {
            value.type = RespValue::Type::Null;
            return true;
        }
        value.type = RespValue::Type::Array;
        for (int i = 0; i < count; ++i) {
            RespValue child;
            if (!parseOne(offset, child)) {
                return false;
            }
            value.array.append(child);
        }
        return true;
    }
    default:
        return false;
    }
}

RedisConnection::RedisConnection(QObject *parent)
    : QObject(parent)
{
    connect(&m_commandSocket, &QTcpSocket::connected, this, &RedisConnection::onCommandConnected);
    connect(&m_subscriberSocket, &QTcpSocket::connected, this, &RedisConnection::onSubscriberConnected);
    connect(&m_commandSocket, &QTcpSocket::readyRead, this, &RedisConnection::onCommandReadyRead);
    connect(&m_subscriberSocket, &QTcpSocket::readyRead, this, &RedisConnection::onSubscriberReadyRead);
    connect(&m_commandSocket, &QTcpSocket::disconnected, this, &RedisConnection::disconnected);
    connect(&m_commandSocket, &QTcpSocket::errorOccurred, this, &RedisConnection::onSocketError);
    connect(&m_subscriberSocket, &QTcpSocket::errorOccurred, this, &RedisConnection::onSocketError);
}

void RedisConnection::connectToRedis(const RedisConfig &config)
{
    disconnectFromRedis();
    m_config = config;
    m_commandReady = false;
    m_subscriberReady = false;
    emit statusChanged(QString("Connecting to %1:%2").arg(config.host).arg(config.port));
    m_commandSocket.connectToHost(config.host, config.port);
    m_subscriberSocket.connectToHost(config.host, config.port);
}

void RedisConnection::disconnectFromRedis()
{
    m_subscriptions.clear();
    m_commandSocket.abort();
    m_subscriberSocket.abort();
    m_commandReady = false;
    m_subscriberReady = false;
}

bool RedisConnection::isConnected() const
{
    return m_commandReady && m_subscriberReady;
}

void RedisConnection::refreshChannels()
{
    if (!m_commandReady) {
        return;
    }
    send(&m_commandSocket, {"PUBSUB", "CHANNELS"});
}

void RedisConnection::subscribe(const QString &channel)
{
    if (channel.trimmed().isEmpty() || !m_subscriberReady) {
        return;
    }
    send(&m_subscriberSocket, {"SUBSCRIBE", channel.toUtf8()});
}

void RedisConnection::unsubscribe(const QString &channel)
{
    if (channel.trimmed().isEmpty() || !m_subscriberReady) {
        return;
    }
    send(&m_subscriberSocket, {"UNSUBSCRIBE", channel.toUtf8()});
}

void RedisConnection::publish(const QString &channel, const QString &message)
{
    if (channel.trimmed().isEmpty() || !m_commandReady) {
        return;
    }
    send(&m_commandSocket, {"PUBLISH", channel.toUtf8(), message.toUtf8()});
}

QByteArray RedisConnection::command(const QList<QByteArray> &parts) const
{
    QByteArray result = "*" + QByteArray::number(parts.size()) + "\r\n";
    for (const QByteArray &part : parts) {
        result += "$" + QByteArray::number(part.size()) + "\r\n" + part + "\r\n";
    }
    return result;
}

void RedisConnection::send(QTcpSocket *socket, const QList<QByteArray> &parts)
{
    socket->write(command(parts));
}

void RedisConnection::authenticate(QTcpSocket *socket)
{
    if (m_config.password.isEmpty()) {
        return;
    }
    if (m_config.username.isEmpty()) {
        send(socket, {"AUTH", m_config.password.toUtf8()});
    } else {
        send(socket, {"AUTH", m_config.username.toUtf8(), m_config.password.toUtf8()});
    }
}

void RedisConnection::selectDatabase(QTcpSocket *socket)
{
    if (m_config.database > 0) {
        send(socket, {"SELECT", QByteArray::number(m_config.database)});
    }
}

void RedisConnection::onCommandConnected()
{
    authenticate(&m_commandSocket);
    selectDatabase(&m_commandSocket);
    send(&m_commandSocket, {"PING"});
}

void RedisConnection::onSubscriberConnected()
{
    authenticate(&m_subscriberSocket);
    selectDatabase(&m_subscriberSocket);
    send(&m_subscriberSocket, {"PING"});
}

void RedisConnection::onCommandReadyRead()
{
    m_commandParser.append(m_commandSocket.readAll());
    for (const RespValue &value : m_commandParser.takeValues()) {
        handleCommandValue(value);
    }
}

void RedisConnection::onSubscriberReadyRead()
{
    m_subscriberParser.append(m_subscriberSocket.readAll());
    for (const RespValue &value : m_subscriberParser.takeValues()) {
        handleSubscriberValue(value);
    }
}

void RedisConnection::onSocketError(QAbstractSocket::SocketError)
{
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    if (socket && socket->error() != QAbstractSocket::RemoteHostClosedError) {
        fail(socket->errorString());
    }
}

void RedisConnection::handleCommandValue(const RespValue &value)
{
    if (value.isError()) {
        fail(value.text);
        return;
    }
    if (!m_commandReady) {
        if (valueToString(value).compare("PONG", Qt::CaseInsensitive) == 0) {
            m_commandReady = true;
            if (isConnected()) {
                emit connected();
                refreshChannels();
            }
        }
        return;
    }
    if (value.type == RespValue::Type::Array) {
        QStringList channels;
        for (const RespValue &item : value.array) {
            channels.append(valueToString(item));
        }
        emit channelsReceived(channels);
    }
}

void RedisConnection::handleSubscriberValue(const RespValue &value)
{
    if (value.isError()) {
        fail(value.text);
        return;
    }
    if (!m_subscriberReady) {
        if (valueToString(value).compare("PONG", Qt::CaseInsensitive) == 0) {
            m_subscriberReady = true;
            if (isConnected()) {
                emit connected();
                refreshChannels();
            }
        }
        return;
    }
    if (value.type != RespValue::Type::Array || value.array.size() < 3) {
        return;
    }

    const QString kind = valueToString(value.array.at(0)).toLower();
    const QString channel = valueToString(value.array.at(1));
    if (kind == "message") {
        emit messageReceived(channel, valueToString(value.array.at(2)));
    } else if (kind == "subscribe") {
        m_subscriptions.insert(channel);
        emit subscribed(channel);
    } else if (kind == "unsubscribe") {
        m_subscriptions.remove(channel);
        emit unsubscribed(channel);
    }
}

QString RedisConnection::valueToString(const RespValue &value) const
{
    if (value.type == RespValue::Type::Integer) {
        return QString::number(value.integer);
    }
    return value.text;
}

void RedisConnection::fail(const QString &message)
{
    emit errorOccurred(message);
    emit statusChanged(message);
}

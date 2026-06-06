#pragma once

#include "RespValue.h"

#include <QObject>
#include <QByteArray>
#include <QHostAddress>
#include <QSet>
#include <QTcpSocket>

struct RedisConfig {
    QString host = "127.0.0.1";
    quint16 port = 6379;
    QString username;
    QString password;
    int database = 0;
};

class RespParser {
public:
    void append(const QByteArray &data);
    QList<RespValue> takeValues();

private:
    bool parseOne(int &offset, RespValue &value) const;
    int lineEnd(int offset) const;

    QByteArray m_buffer;
};

class RedisConnection : public QObject {
    Q_OBJECT

public:
    explicit RedisConnection(QObject *parent = nullptr);

    void connectToRedis(const RedisConfig &config);
    void disconnectFromRedis();
    bool isConnected() const;

    void refreshChannels();
    void subscribe(const QString &channel);
    void unsubscribe(const QString &channel);
    void publish(const QString &channel, const QString &message);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &message);
    void channelsReceived(const QStringList &channels);
    void subscribed(const QString &channel);
    void unsubscribed(const QString &channel);
    void messageReceived(const QString &channel, const QString &message);
    void statusChanged(const QString &message);

private slots:
    void onCommandConnected();
    void onSubscriberConnected();
    void onCommandReadyRead();
    void onSubscriberReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);

private:
    enum class SocketRole {
        Command,
        Subscriber
    };

    QByteArray command(const QList<QByteArray> &parts) const;
    void send(QTcpSocket *socket, const QList<QByteArray> &parts);
    void authenticate(QTcpSocket *socket);
    void selectDatabase(QTcpSocket *socket);
    void handleCommandValue(const RespValue &value);
    void handleSubscriberValue(const RespValue &value);
    QString valueToString(const RespValue &value) const;
    void fail(const QString &message);

    RedisConfig m_config;
    QTcpSocket m_commandSocket;
    QTcpSocket m_subscriberSocket;
    RespParser m_commandParser;
    RespParser m_subscriberParser;
    bool m_commandReady = false;
    bool m_subscriberReady = false;
    QSet<QString> m_subscriptions;
};

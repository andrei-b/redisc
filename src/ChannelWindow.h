#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QTextEdit;

class ChannelWindow : public QWidget {
    Q_OBJECT

public:
    explicit ChannelWindow(const QString &channel, QWidget *parent = nullptr);

    QString channel() const;
    void appendMessage(const QString &message);

signals:
    void publishRequested(const QString &channel, const QString &message);
    void unsubscribeRequested(const QString &channel);

private:
    QString m_channel;
    QTextEdit *m_messages = nullptr;
    QLineEdit *m_input = nullptr;
    QPushButton *m_send = nullptr;
};

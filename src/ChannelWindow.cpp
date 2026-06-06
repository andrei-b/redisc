#include "ChannelWindow.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QToolButton>
#include <QVBoxLayout>

ChannelWindow::ChannelWindow(const QString &channel, QWidget *parent)
    : QWidget(parent)
    , m_channel(channel)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    m_messages = new QTextEdit(this);
    m_messages->setReadOnly(true);

    auto *row = new QHBoxLayout;
    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("Message");
    m_send = new QPushButton("Publish", this);
    auto *unsubscribe = new QToolButton(this);
    unsubscribe->setText("x");
    unsubscribe->setToolTip("Unsubscribe");

    row->addWidget(m_input, 1);
    row->addWidget(m_send);
    row->addWidget(unsubscribe);

    layout->addWidget(m_messages, 1);
    layout->addLayout(row);

    connect(m_send, &QPushButton::clicked, this, [this]() {
        const QString message = m_input->text();
        if (!message.isEmpty()) {
            emit publishRequested(m_channel, message);
            m_input->clear();
        }
    });
    connect(m_input, &QLineEdit::returnPressed, m_send, &QPushButton::click);
    connect(unsubscribe, &QToolButton::clicked, this, [this]() {
        emit unsubscribeRequested(m_channel);
    });
}

QString ChannelWindow::channel() const
{
    return m_channel;
}

void ChannelWindow::appendMessage(const QString &message)
{
    const QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_messages->append(QString("[%1] %2").arg(time, message.toHtmlEscaped()));
}

#include "MainWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Redisc");
    resize(1180, 760);

    m_mdi = new QMdiArea(this);
    setCentralWidget(m_mdi);

    auto *dock = new QDockWidget("Connection", this);
    dock->setObjectName("connectionDock");
    auto *tabs = new QTabWidget(dock);
    tabs->addTab(buildConnectionPanel(), "Login");
    tabs->addTab(buildChannelPanel(), "Channels");
    dock->setWidget(tabs);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    statusBar()->showMessage("Disconnected");

    connect(&m_redis, &RedisConnection::connected, this, &MainWindow::onConnected);
    connect(&m_redis, &RedisConnection::disconnected, this, [this]() {
        setUiConnected(false);
        statusBar()->showMessage("Disconnected");
    });
    connect(&m_redis, &RedisConnection::errorOccurred, this, [this](const QString &message) {
        statusBar()->showMessage(message);
        QMessageBox::warning(this, "Redis error", message);
    });
    connect(&m_redis, &RedisConnection::channelsReceived, this, &MainWindow::onChannelsReceived);
    connect(&m_redis, &RedisConnection::subscribed, this, &MainWindow::onSubscribed);
    connect(&m_redis, &RedisConnection::unsubscribed, this, &MainWindow::onUnsubscribed);
    connect(&m_redis, &RedisConnection::messageReceived, this, &MainWindow::onMessage);

    connect(&m_tunnel, &SshTunnel::ready, this, [this]() {
        m_redis.connectToRedis(redisConfig());
    });
    connect(&m_tunnel, &SshTunnel::errorOccurred, this, [this](const QString &message) {
        QMessageBox::warning(this, "SSH tunnel error", message);
    });

    applyTheme("System");
}

QWidget *MainWindow::buildConnectionPanel()
{
    auto *panel = new QWidget(this);
    auto *layout = new QVBoxLayout(panel);

    auto *redisBox = new QGroupBox("Redis", panel);
    auto *redisForm = new QFormLayout(redisBox);
    m_host = new QLineEdit("127.0.0.1", redisBox);
    m_port = new QSpinBox(redisBox);
    m_port->setRange(1, 65535);
    m_port->setValue(6379);
    m_username = new QLineEdit(redisBox);
    m_password = new QLineEdit(redisBox);
    m_password->setEchoMode(QLineEdit::Password);
    m_database = new QSpinBox(redisBox);
    m_database->setRange(0, 15);
    redisForm->addRow("Host", m_host);
    redisForm->addRow("Port", m_port);
    redisForm->addRow("Username", m_username);
    redisForm->addRow("Password", m_password);
    redisForm->addRow("Database", m_database);

    auto *sshBox = new QGroupBox("SSH tunnel", panel);
    auto *sshForm = new QFormLayout(sshBox);
    m_useSsh = new QCheckBox("Use SSH tunnel", sshBox);
    m_sshHost = new QLineEdit(sshBox);
    m_sshPort = new QSpinBox(sshBox);
    m_sshPort->setRange(1, 65535);
    m_sshPort->setValue(22);
    m_sshUser = new QLineEdit(sshBox);
    m_identityFile = new QLineEdit(sshBox);
    auto *identityRow = new QWidget(sshBox);
    auto *identityLayout = new QHBoxLayout(identityRow);
    identityLayout->setContentsMargins(0, 0, 0, 0);
    auto *browseIdentity = new QToolButton(identityRow);
    browseIdentity->setText("...");
    browseIdentity->setToolTip("Choose identity file");
    identityLayout->addWidget(m_identityFile, 1);
    identityLayout->addWidget(browseIdentity);
    m_localPort = new QSpinBox(sshBox);
    m_localPort->setRange(1, 65535);
    m_localPort->setValue(16379);
    sshForm->addRow(m_useSsh);
    sshForm->addRow("SSH host", m_sshHost);
    sshForm->addRow("SSH port", m_sshPort);
    sshForm->addRow("SSH user", m_sshUser);
    sshForm->addRow("Identity", identityRow);
    sshForm->addRow("Local port", m_localPort);

    auto *buttons = new QHBoxLayout;
    auto *connectButton = new QPushButton("Connect", panel);
    auto *disconnectButton = new QPushButton("Disconnect", panel);
    buttons->addWidget(connectButton);
    buttons->addWidget(disconnectButton);

    m_theme = new QComboBox(panel);
    m_theme->addItems({"System", "Light", "Dark", "Solarized"});

    layout->addWidget(redisBox);
    layout->addWidget(sshBox);
    layout->addLayout(buttons);
    layout->addWidget(new QLabel("Color scheme", panel));
    layout->addWidget(m_theme);
    layout->addStretch(1);

    connect(connectButton, &QPushButton::clicked, this, &MainWindow::connectClicked);
    connect(disconnectButton, &QPushButton::clicked, this, &MainWindow::disconnectClicked);
    connect(browseIdentity, &QToolButton::clicked, this, [this]() {
        const QString file = QFileDialog::getOpenFileName(this, "Identity file");
        if (!file.isEmpty()) {
            m_identityFile->setText(file);
        }
    });
    connect(m_theme, &QComboBox::currentTextChanged, this, &MainWindow::applyTheme);

    return panel;
}

QWidget *MainWindow::buildChannelPanel()
{
    auto *panel = new QWidget(this);
    auto *layout = new QVBoxLayout(panel);
    auto *refresh = new QPushButton("Refresh channels", panel);
    m_channelList = new QListWidget(panel);
    m_manualChannel = new QLineEdit(panel);
    m_manualChannel->setPlaceholderText("Channel name");
    auto *subscribe = new QPushButton("Open channel", panel);

    layout->addWidget(refresh);
    layout->addWidget(m_channelList, 1);
    layout->addWidget(m_manualChannel);
    layout->addWidget(subscribe);

    connect(refresh, &QPushButton::clicked, &m_redis, &RedisConnection::refreshChannels);
    connect(m_channelList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        openChannel(item->text());
    });
    connect(subscribe, &QPushButton::clicked, this, [this]() {
        QString channel = m_manualChannel->text().trimmed();
        if (channel.isEmpty() && m_channelList->currentItem()) {
            channel = m_channelList->currentItem()->text();
        }
        openChannel(channel);
    });
    connect(m_manualChannel, &QLineEdit::returnPressed, subscribe, &QPushButton::click);

    return panel;
}

void MainWindow::connectClicked()
{
    if (m_host->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Missing Redis host", "Enter a Redis host before connecting.");
        return;
    }
    if (m_useSsh->isChecked() && m_sshHost->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Missing SSH host", "Enter an SSH host before connecting over a tunnel.");
        return;
    }
    statusBar()->showMessage("Connecting...");
    if (m_useSsh->isChecked()) {
        m_tunnel.start(tunnelConfig());
    } else {
        m_redis.connectToRedis(redisConfig());
    }
}

void MainWindow::disconnectClicked()
{
    m_redis.disconnectFromRedis();
    m_tunnel.stop();
    setUiConnected(false);
}

void MainWindow::onConnected()
{
    setUiConnected(true);
    statusBar()->showMessage("Connected");
}

void MainWindow::onChannelsReceived(const QStringList &channels)
{
    m_channelList->clear();
    for (const QString &channel : channels) {
        m_channelList->addItem(channel);
    }
}

void MainWindow::openChannel(const QString &channel)
{
    const QString trimmed = channel.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    if (m_windows.contains(trimmed)) {
        m_mdi->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(m_windows.value(trimmed)->parentWidget()));
        return;
    }
    m_redis.subscribe(trimmed);
}

void MainWindow::onSubscribed(const QString &channel)
{
    if (m_windows.contains(channel)) {
        return;
    }
    auto *window = new ChannelWindow(channel);
    auto *subWindow = m_mdi->addSubWindow(window);
    subWindow->setWindowTitle(channel);
    subWindow->resize(520, 360);
    window->show();
    m_windows.insert(channel, window);

    connect(window, &ChannelWindow::publishRequested, &m_redis, &RedisConnection::publish);
    connect(window, &ChannelWindow::unsubscribeRequested, &m_redis, &RedisConnection::unsubscribe);
    connect(subWindow, &QMdiSubWindow::destroyed, this, [this, channel]() {
        m_windows.remove(channel);
    });
}

void MainWindow::onUnsubscribed(const QString &channel)
{
    if (auto *window = m_windows.take(channel)) {
        window->parentWidget()->close();
    }
}

void MainWindow::onMessage(const QString &channel, const QString &message)
{
    if (!m_windows.contains(channel)) {
        onSubscribed(channel);
    }
    m_windows.value(channel)->appendMessage(message);
}

RedisConfig MainWindow::redisConfig() const
{
    RedisConfig config;
    config.host = m_useSsh->isChecked() ? "127.0.0.1" : m_host->text().trimmed();
    config.port = static_cast<quint16>(m_useSsh->isChecked() ? m_localPort->value() : m_port->value());
    config.username = m_username->text().trimmed();
    config.password = m_password->text();
    config.database = m_database->value();
    return config;
}

SshTunnelConfig MainWindow::tunnelConfig() const
{
    SshTunnelConfig config;
    config.enabled = m_useSsh->isChecked();
    config.sshHost = m_sshHost->text().trimmed();
    config.sshPort = static_cast<quint16>(m_sshPort->value());
    config.sshUser = m_sshUser->text().trimmed();
    config.identityFile = m_identityFile->text().trimmed();
    config.remoteHost = m_host->text().trimmed();
    config.remotePort = static_cast<quint16>(m_port->value());
    config.localPort = static_cast<quint16>(m_localPort->value());
    return config;
}

void MainWindow::setUiConnected(bool connected)
{
    statusBar()->showMessage(connected ? "Connected" : "Disconnected");
}

void MainWindow::applyTheme(const QString &theme)
{
    QString sheet;
    if (theme == "Dark") {
        sheet = R"(
            QWidget { background: #202124; color: #f1f3f4; }
            QLineEdit, QSpinBox, QListWidget, QTextEdit, QMdiArea, QComboBox {
                background: #2d2f33; border: 1px solid #5f6368; selection-background-color: #3c7dd9;
            }
            QPushButton, QToolButton { background: #35373b; border: 1px solid #6b7078; padding: 6px 10px; }
            QPushButton:hover, QToolButton:hover { background: #44474d; }
            QDockWidget::title { padding: 6px; }
        )";
    } else if (theme == "Light") {
        sheet = R"(
            QWidget { background: #f7f8fa; color: #202124; }
            QLineEdit, QSpinBox, QListWidget, QTextEdit, QMdiArea, QComboBox {
                background: #ffffff; border: 1px solid #c7ccd4; selection-background-color: #2f80ed;
            }
            QPushButton, QToolButton { background: #ffffff; border: 1px solid #b8bec8; padding: 6px 10px; }
            QPushButton:hover, QToolButton:hover { background: #eef2f7; }
        )";
    } else if (theme == "Solarized") {
        sheet = R"(
            QWidget { background: #fdf6e3; color: #073642; }
            QLineEdit, QSpinBox, QListWidget, QTextEdit, QMdiArea, QComboBox {
                background: #eee8d5; border: 1px solid #93a1a1; selection-background-color: #268bd2;
            }
            QPushButton, QToolButton { background: #eee8d5; border: 1px solid #839496; padding: 6px 10px; }
            QPushButton:hover, QToolButton:hover { background: #e1dbc7; }
        )";
    }
    qApp->setStyleSheet(sheet);
}

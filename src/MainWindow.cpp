#include "MainWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QBrush>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setObjectName("mainWindow");
    setWindowTitle("Redisc");
    resize(1180, 760);

    m_mdi = new QMdiArea(this);
    m_mdi->setObjectName("mainWorkspace");
    setCentralWidget(m_mdi);
    connect(m_mdi, &QMdiArea::subWindowActivated, this, &MainWindow::updateChannelWindowTitles);

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
    connect(&m_python, &PythonClient::publishRequested, this, &MainWindow::publishFromPython);
    connect(&m_python, &PythonClient::subscribeRequested, this, &MainWindow::openChannel);
    connect(&m_python, &PythonClient::unsubscribeRequested, &m_redis, &RedisConnection::unsubscribe);
    connect(&m_python, &PythonClient::logMessage, this, [this](const QString &message) {
        statusBar()->showMessage(message);
    });
    connect(&m_python, &PythonClient::errorOccurred, this, [this](const QString &message) {
        statusBar()->showMessage(message);
        QMessageBox::warning(this, "Python", message);
    });

    loadSettings();
    applyTheme(m_theme->currentText());
    if (m_pythonEnabled->isChecked() && !m_pythonScript->text().trimmed().isEmpty()) {
        loadPythonScript();
    }
}

QWidget *MainWindow::buildConnectionPanel()
{
    auto *panel = new QWidget(this);
    auto *layout = new QVBoxLayout(panel);

    auto *profilesBox = new QGroupBox("Connections", panel);
    auto *profilesLayout = new QVBoxLayout(profilesBox);
    m_connectionList = new QListWidget(profilesBox);
    m_connectionName = new QLineEdit(profilesBox);
    m_connectionName->setPlaceholderText("Connection name");
    auto *profileButtons = new QHBoxLayout;
    auto *newProfile = new QPushButton("New", profilesBox);
    auto *saveProfile = new QPushButton("Save", profilesBox);
    auto *deleteProfile = new QPushButton("Delete", profilesBox);
    profileButtons->addWidget(newProfile);
    profileButtons->addWidget(saveProfile);
    profileButtons->addWidget(deleteProfile);
    profilesLayout->addWidget(m_connectionList);
    profilesLayout->addWidget(m_connectionName);
    profilesLayout->addLayout(profileButtons);

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

    auto *sshBox = new QGroupBox("[+] SSH tunnel", panel);
    sshBox->setCheckable(true);
    sshBox->setChecked(false);
    auto *sshBoxLayout = new QVBoxLayout(sshBox);
    auto *sshBody = new QWidget(sshBox);
    auto *sshForm = new QFormLayout(sshBody);
    m_useSsh = new QCheckBox("Use SSH tunnel", sshBody);
    m_sshHost = new QLineEdit(sshBody);
    m_sshPort = new QSpinBox(sshBody);
    m_sshPort->setRange(1, 65535);
    m_sshPort->setValue(22);
    m_sshUser = new QLineEdit(sshBody);
    m_identityFile = new QLineEdit(sshBody);
    auto *identityRow = new QWidget(sshBody);
    auto *identityLayout = new QHBoxLayout(identityRow);
    identityLayout->setContentsMargins(0, 0, 0, 0);
    auto *browseIdentity = new QToolButton(identityRow);
    browseIdentity->setText("...");
    browseIdentity->setToolTip("Choose identity file");
    identityLayout->addWidget(m_identityFile, 1);
    identityLayout->addWidget(browseIdentity);
    m_localPort = new QSpinBox(sshBody);
    m_localPort->setRange(1, 65535);
    m_localPort->setValue(16379);
    sshForm->addRow(m_useSsh);
    sshForm->addRow("SSH host", m_sshHost);
    sshForm->addRow("SSH port", m_sshPort);
    sshForm->addRow("SSH user", m_sshUser);
    sshForm->addRow("Identity", identityRow);
    sshForm->addRow("Local port", m_localPort);
    sshBoxLayout->addWidget(sshBody);
    sshBody->setVisible(false);

    auto *pythonBox = new QGroupBox("[+] Python client", panel);
    pythonBox->setCheckable(true);
    pythonBox->setChecked(false);
    auto *pythonBoxLayout = new QVBoxLayout(pythonBox);
    auto *pythonBody = new QWidget(pythonBox);
    auto *pythonForm = new QFormLayout(pythonBody);
    m_pythonEnabled = new QCheckBox("Notify Python on Redis messages", pythonBody);
    m_pythonScript = new QLineEdit(pythonBody);
    auto *scriptRow = new QWidget(pythonBody);
    auto *scriptLayout = new QHBoxLayout(scriptRow);
    scriptLayout->setContentsMargins(0, 0, 0, 0);
    auto *browseScript = new QToolButton(scriptRow);
    browseScript->setText("...");
    browseScript->setToolTip("Choose Python script");
    scriptLayout->addWidget(m_pythonScript, 1);
    scriptLayout->addWidget(browseScript);
    auto *pythonButtons = new QHBoxLayout;
    auto *loadScript = new QPushButton("Load", pythonBody);
    auto *editScript = new QPushButton("Edit", pythonBody);
    auto *unloadScript = new QPushButton("Unload", pythonBody);
    pythonButtons->addWidget(loadScript);
    pythonButtons->addWidget(editScript);
    pythonButtons->addWidget(unloadScript);
    pythonForm->addRow(m_pythonEnabled);
    pythonForm->addRow("Script", scriptRow);
    pythonForm->addRow(pythonButtons);
    pythonBoxLayout->addWidget(pythonBody);
    pythonBody->setVisible(false);

    auto *buttons = new QHBoxLayout;
    auto *connectButton = new QPushButton("Connect", panel);
    auto *disconnectButton = new QPushButton("Disconnect", panel);
    buttons->addWidget(connectButton);
    buttons->addWidget(disconnectButton);

    m_theme = new QComboBox(panel);
    loadThemes();

    layout->addWidget(profilesBox);
    layout->addWidget(redisBox);
    layout->addWidget(sshBox);
    layout->addWidget(pythonBox);
    layout->addLayout(buttons);
    layout->addWidget(new QLabel("Color scheme", panel));
    layout->addWidget(m_theme);
    layout->addStretch(1);

    connect(connectButton, &QPushButton::clicked, this, &MainWindow::connectClicked);
    connect(disconnectButton, &QPushButton::clicked, this, &MainWindow::disconnectClicked);
    connect(newProfile, &QPushButton::clicked, this, [this]() {
        m_connectionList->clearSelection();
        resetConnectionFields();
        m_connectionName->setFocus();
    });
    connect(saveProfile, &QPushButton::clicked, this, &MainWindow::saveCurrentConnectionProfile);
    connect(deleteProfile, &QPushButton::clicked, this, &MainWindow::deleteCurrentConnectionProfile);
    connect(m_connectionList, &QListWidget::currentTextChanged, this, &MainWindow::loadConnectionProfile);
    connect(sshBox, &QGroupBox::toggled, this, [sshBox, sshBody](bool expanded) {
        sshBody->setVisible(expanded);
        sshBox->setTitle(expanded ? "[-] SSH tunnel" : "[+] SSH tunnel");
    });
    connect(browseIdentity, &QToolButton::clicked, this, [this]() {
        const QString file = QFileDialog::getOpenFileName(this, "Identity file");
        if (!file.isEmpty()) {
            m_identityFile->setText(file);
        }
    });
    connect(pythonBox, &QGroupBox::toggled, this, [pythonBox, pythonBody](bool expanded) {
        pythonBody->setVisible(expanded);
        pythonBox->setTitle(expanded ? "[-] Python client" : "[+] Python client");
    });
    connect(browseScript, &QToolButton::clicked, this, [this]() {
        const QString file = QFileDialog::getOpenFileName(this, "Python script", QString(), "Python files (*.py);;All files (*)");
        if (!file.isEmpty()) {
            m_pythonScript->setText(file);
        }
    });
    connect(loadScript, &QPushButton::clicked, this, &MainWindow::loadPythonScript);
    connect(editScript, &QPushButton::clicked, this, &MainWindow::openPythonEditor);
    connect(unloadScript, &QPushButton::clicked, this, [this]() {
        m_python.clear();
        statusBar()->showMessage("Unloaded Python script");
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
    auto *appearanceBox = new QGroupBox("Channel window text", panel);
    auto *appearanceLayout = new QFormLayout(appearanceBox);
    m_channelFontButton = new QPushButton("Font", appearanceBox);
    m_channelTextColorButton = new QPushButton("Color", appearanceBox);
    m_channelTextColorPreview = new QLabel(appearanceBox);
    m_channelTextColorPreview->setFixedSize(42, 22);
    m_channelTextColorPreview->setFrameShape(QFrame::Panel);
    m_channelTextColorPreview->setAutoFillBackground(true);
    auto *colorRow = new QWidget(appearanceBox);
    auto *colorLayout = new QHBoxLayout(colorRow);
    colorLayout->setContentsMargins(0, 0, 0, 0);
    colorLayout->addWidget(m_channelTextColorButton);
    colorLayout->addWidget(m_channelTextColorPreview);
    colorLayout->addStretch(1);
    appearanceLayout->addRow("Font", m_channelFontButton);
    appearanceLayout->addRow("Text color", colorRow);

    layout->addWidget(refresh);
    layout->addWidget(m_channelList, 1);
    layout->addWidget(m_manualChannel);
    layout->addWidget(subscribe);
    layout->addWidget(appearanceBox);

    connect(refresh, &QPushButton::clicked, &m_redis, &RedisConnection::refreshChannels);
    connect(m_channelList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        openChannel(item->data(Qt::UserRole).toString());
    });
    connect(subscribe, &QPushButton::clicked, this, [this]() {
        QString channel = m_manualChannel->text().trimmed();
        if (channel.isEmpty() && m_channelList->currentItem()) {
            channel = m_channelList->currentItem()->data(Qt::UserRole).toString();
        }
        openChannel(channel);
    });
    connect(m_manualChannel, &QLineEdit::returnPressed, subscribe, &QPushButton::click);
    connect(m_channelFontButton, &QPushButton::clicked, this, &MainWindow::chooseChannelFont);
    connect(m_channelTextColorButton, &QPushButton::clicked, this, &MainWindow::chooseChannelTextColor);

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
    if (!currentConnectionName().isEmpty()) {
        saveCurrentConnectionProfile();
    }
    saveSettings();
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
    m_liveChannels = channels;
    updateChannelList();
}

void MainWindow::openChannel(const QString &channel)
{
    const QString trimmed = channel.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    rememberChannel(trimmed);
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
    window->setMessageAppearance(m_channelFont, m_channelTextColor);
    auto *subWindow = m_mdi->addSubWindow(window);
    subWindow->setWindowTitle(channel);
    subWindow->setProperty("channelName", channel);
    subWindow->resize(520, 360);
    window->show();
    m_windows.insert(channel, window);
    updateChannelWindowTitles();

    connect(window, &ChannelWindow::publishRequested, &m_redis, &RedisConnection::publish);
    connect(window, &ChannelWindow::unsubscribeRequested, &m_redis, &RedisConnection::unsubscribe);
    connect(window, &ChannelWindow::jsonViewRequested, this, &MainWindow::openJsonViewer);
    connect(subWindow, &QMdiSubWindow::destroyed, this, [this, channel]() {
        m_windows.remove(channel);
        updateChannelWindowTitles();
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
    if (m_pythonEnabled->isChecked()) {
        if (!m_python.isLoaded() && !m_pythonScript->text().trimmed().isEmpty()) {
            loadPythonScript();
        }
        if (m_python.isLoaded()) {
            m_pythonWarnedNotLoaded = false;
            m_python.notifyRedisMessage(channel, message);
        } else if (!m_pythonWarnedNotLoaded) {
            m_pythonWarnedNotLoaded = true;
            statusBar()->showMessage("Python notifications are enabled, but no script is loaded.");
        }
    }
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
    QString sheet = m_themeStyles.value(theme);
    const QString mainWindowBackground = m_themeMainWindowBackgrounds.value(theme);
    qApp->setStyleSheet(sheet);
    if (!mainWindowBackground.isEmpty()) {
        const QColor color(mainWindowBackground);
        if (color.isValid()) {
            m_mdi->setBackground(QBrush(color));
        }
    } else {
        m_mdi->setBackground(QBrush());
    }
    updateChannelWindowTitles();
    saveSettings();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::loadSettings()
{
    QSettings settings;

    m_recentChannels = settings.value("channels/recent").toStringList();
    m_pythonEnabled->setChecked(settings.value("python/enabled", false).toBool());
    m_pythonScript->setText(settings.value("python/script").toString());
    m_channelFont = QApplication::font();
    const QString fontValue = settings.value("channels/font").toString();
    if (!fontValue.isEmpty()) {
        m_channelFont.fromString(fontValue);
    }
    m_channelTextColor = QColor(settings.value("channels/textColor", "#202124").toString());
    if (!m_channelTextColor.isValid()) {
        m_channelTextColor = QColor("#202124");
    }
    updateChannelList();
    updateChannelAppearanceControls();

    loadConnectionProfiles();

    const QString theme = settings.value("ui/theme", "System").toString();
    const int themeIndex = m_theme->findText(theme);
    {
        const QSignalBlocker blocker(m_theme);
        m_theme->setCurrentIndex(themeIndex >= 0 ? themeIndex : 0);
    }

    const QByteArray geometry = settings.value("ui/geometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
}

void MainWindow::saveSettings() const
{
    QSettings settings;

    settings.setValue("connections/last", currentConnectionName());
    settings.setValue("channels/recent", m_recentChannels);
    settings.setValue("python/enabled", m_pythonEnabled->isChecked());
    settings.setValue("python/script", m_pythonScript->text().trimmed());
    settings.setValue("channels/font", m_channelFont.toString());
    settings.setValue("channels/textColor", m_channelTextColor.name());
    settings.setValue("ui/theme", m_theme->currentText());
    settings.setValue("ui/geometry", saveGeometry());
}

void MainWindow::loadConnectionProfiles()
{
    QSettings settings;
    QStringList names = settings.value("connections/names").toStringList();

    if (names.isEmpty() && settings.contains("redis/host")) {
        const QVariant redisHost = settings.value("redis/host", "127.0.0.1");
        const QVariant redisPort = settings.value("redis/port", 6379);
        const QVariant redisUsername = settings.value("redis/username");
        const QVariant redisPassword = settings.value("redis/password");
        const QVariant redisDatabase = settings.value("redis/database", 0);
        const QVariant sshEnabled = settings.value("ssh/enabled", false);
        const QVariant sshHost = settings.value("ssh/host");
        const QVariant sshPort = settings.value("ssh/port", 22);
        const QVariant sshUser = settings.value("ssh/user");
        const QVariant sshIdentityFile = settings.value("ssh/identityFile");
        const QVariant sshLocalPort = settings.value("ssh/localPort", 16379);

        names << "Default";
        settings.setValue("connections/names", names);
        settings.beginGroup("connections/profiles/Default");
        settings.setValue("redisHost", redisHost);
        settings.setValue("redisPort", redisPort);
        settings.setValue("redisUsername", redisUsername);
        settings.setValue("redisPassword", redisPassword);
        settings.setValue("redisDatabase", redisDatabase);
        settings.setValue("sshEnabled", sshEnabled);
        settings.setValue("sshHost", sshHost);
        settings.setValue("sshPort", sshPort);
        settings.setValue("sshUser", sshUser);
        settings.setValue("sshIdentityFile", sshIdentityFile);
        settings.setValue("sshLocalPort", sshLocalPort);
        settings.endGroup();
    }

    {
        const QSignalBlocker blocker(m_connectionList);
        m_connectionList->clear();
        for (const QString &name : names) {
            m_connectionList->addItem(name);
        }
    }

    const QString last = settings.value("connections/last").toString();
    const QList<QListWidgetItem *> matches = m_connectionList->findItems(last, Qt::MatchExactly);
    if (!matches.isEmpty()) {
        m_connectionList->setCurrentItem(matches.first());
        loadConnectionProfile(last);
    } else if (m_connectionList->count() > 0) {
        m_connectionList->setCurrentRow(0);
        loadConnectionProfile(m_connectionList->currentItem()->text());
    } else {
        resetConnectionFields();
    }
}

void MainWindow::loadConnectionProfile(const QString &name)
{
    if (name.isEmpty()) {
        return;
    }

    QSettings settings;
    settings.beginGroup("connections/profiles/" + name);
    m_connectionName->setText(name);
    m_host->setText(settings.value("redisHost", "127.0.0.1").toString());
    m_port->setValue(settings.value("redisPort", 6379).toInt());
    m_username->setText(settings.value("redisUsername").toString());
    m_password->setText(settings.value("redisPassword").toString());
    m_database->setValue(settings.value("redisDatabase", 0).toInt());
    m_useSsh->setChecked(settings.value("sshEnabled", false).toBool());
    m_sshHost->setText(settings.value("sshHost").toString());
    m_sshPort->setValue(settings.value("sshPort", 22).toInt());
    m_sshUser->setText(settings.value("sshUser").toString());
    m_identityFile->setText(settings.value("sshIdentityFile").toString());
    m_localPort->setValue(settings.value("sshLocalPort", 16379).toInt());
    settings.endGroup();
}

void MainWindow::saveCurrentConnectionProfile()
{
    const QString name = currentConnectionName();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Missing connection name", "Enter a connection name before saving.");
        return;
    }
    if (name.contains('/') || name.contains('\\')) {
        QMessageBox::warning(this, "Invalid connection name", "Connection names cannot contain slashes.");
        return;
    }

    QSettings settings;
    QStringList names = settings.value("connections/names").toStringList();
    const QString previousName = m_connectionList->currentItem() ? m_connectionList->currentItem()->text() : QString();
    if (!previousName.isEmpty() && previousName != name) {
        settings.remove("connections/profiles/" + previousName);
        const int oldIndex = names.indexOf(previousName);
        if (oldIndex >= 0) {
            names[oldIndex] = name;
        }
    }
    if (!names.contains(name)) {
        names.append(name);
    }

    settings.setValue("connections/names", names);
    settings.setValue("connections/last", name);
    settings.beginGroup("connections/profiles/" + name);
    settings.setValue("redisHost", m_host->text().trimmed());
    settings.setValue("redisPort", m_port->value());
    settings.setValue("redisUsername", m_username->text().trimmed());
    settings.setValue("redisPassword", m_password->text());
    settings.setValue("redisDatabase", m_database->value());
    settings.setValue("sshEnabled", m_useSsh->isChecked());
    settings.setValue("sshHost", m_sshHost->text().trimmed());
    settings.setValue("sshPort", m_sshPort->value());
    settings.setValue("sshUser", m_sshUser->text().trimmed());
    settings.setValue("sshIdentityFile", m_identityFile->text().trimmed());
    settings.setValue("sshLocalPort", m_localPort->value());
    settings.endGroup();

    {
        const QSignalBlocker blocker(m_connectionList);
        m_connectionList->clear();
        for (const QString &profileName : names) {
            m_connectionList->addItem(profileName);
        }
    }
    const QList<QListWidgetItem *> matches = m_connectionList->findItems(name, Qt::MatchExactly);
    if (!matches.isEmpty()) {
        m_connectionList->setCurrentItem(matches.first());
    }
    statusBar()->showMessage(QString("Saved connection '%1'").arg(name));
}

void MainWindow::deleteCurrentConnectionProfile()
{
    const QString name = m_connectionList->currentItem() ? m_connectionList->currentItem()->text() : QString();
    if (name.isEmpty()) {
        return;
    }

    const QMessageBox::StandardButton choice = QMessageBox::question(
        this,
        "Delete connection",
        QString("Delete connection '%1'?").arg(name));
    if (choice != QMessageBox::Yes) {
        return;
    }

    QSettings settings;
    QStringList names = settings.value("connections/names").toStringList();
    names.removeAll(name);
    settings.setValue("connections/names", names);
    settings.remove("connections/profiles/" + name);
    if (settings.value("connections/last").toString() == name) {
        settings.remove("connections/last");
    }

    loadConnectionProfiles();
    statusBar()->showMessage(QString("Deleted connection '%1'").arg(name));
}

void MainWindow::resetConnectionFields()
{
    m_connectionName->clear();
    m_host->setText("127.0.0.1");
    m_port->setValue(6379);
    m_username->clear();
    m_password->clear();
    m_database->setValue(0);
    m_useSsh->setChecked(false);
    m_sshHost->clear();
    m_sshPort->setValue(22);
    m_sshUser->clear();
    m_identityFile->clear();
    m_localPort->setValue(16379);
}

QString MainWindow::currentConnectionName() const
{
    return m_connectionName->text().trimmed();
}

void MainWindow::rememberChannel(const QString &channel)
{
    const QString trimmed = channel.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    m_recentChannels.removeAll(trimmed);
    m_recentChannels.prepend(trimmed);
    while (m_recentChannels.size() > 100) {
        m_recentChannels.removeLast();
    }
    updateChannelList();
    saveSettings();
}

void MainWindow::updateChannelList()
{
    if (!m_channelList) {
        return;
    }

    m_channelList->clear();
    QStringList added;
    auto addChannel = [this, &added](const QString &channel, const QString &suffix) {
        const QString trimmed = channel.trimmed();
        if (trimmed.isEmpty() || added.contains(trimmed)) {
            return;
        }
        auto *item = new QListWidgetItem(suffix.isEmpty() ? trimmed : QString("%1 %2").arg(trimmed, suffix));
        item->setData(Qt::UserRole, trimmed);
        m_channelList->addItem(item);
        added.append(trimmed);
    };

    for (const QString &channel : m_recentChannels) {
        addChannel(channel, "(recent)");
    }
    for (const QString &channel : m_liveChannels) {
        addChannel(channel, QString());
    }
}

void MainWindow::chooseChannelFont()
{
    bool ok = false;
    const QFont font = QFontDialog::getFont(&ok, m_channelFont, this, "Channel window font");
    if (!ok) {
        return;
    }
    m_channelFont = font;
    updateChannelAppearanceControls();
    applyChannelAppearance();
    saveSettings();
}

void MainWindow::chooseChannelTextColor()
{
    const QColor color = QColorDialog::getColor(m_channelTextColor, this, "Channel window text color");
    if (!color.isValid()) {
        return;
    }
    m_channelTextColor = color;
    updateChannelAppearanceControls();
    applyChannelAppearance();
    saveSettings();
}

void MainWindow::applyChannelAppearance()
{
    for (ChannelWindow *window : std::as_const(m_windows)) {
        window->setMessageAppearance(m_channelFont, m_channelTextColor);
    }
}

void MainWindow::updateChannelAppearanceControls()
{
    if (m_channelFontButton) {
        m_channelFontButton->setText(QString("%1 %2").arg(m_channelFont.family()).arg(m_channelFont.pointSize()));
    }
    if (m_channelTextColorPreview) {
        QPalette palette = m_channelTextColorPreview->palette();
        palette.setColor(QPalette::Window, m_channelTextColor);
        m_channelTextColorPreview->setPalette(palette);
        m_channelTextColorPreview->setToolTip(m_channelTextColor.name());
    }
}

void MainWindow::openJsonViewer(const QString &channel, const QString &jsonText)
{
    auto *viewer = new JsonTreeWindow(channel, jsonText);
    auto *subWindow = m_mdi->addSubWindow(viewer);
    ++m_jsonWindowCount;
    subWindow->setWindowTitle(QString("%1 JSON %2").arg(channel).arg(m_jsonWindowCount));
    subWindow->resize(620, 520);
    viewer->show();
    m_mdi->setActiveSubWindow(subWindow);
}

void MainWindow::loadPythonScript()
{
    if (m_python.loadScript(m_pythonScript->text())) {
        m_pythonEnabled->setChecked(true);
        m_pythonWarnedNotLoaded = false;
        saveSettings();
    }
}

void MainWindow::publishFromPython(const QString &channel, const QString &message)
{
    const QString trimmed = channel.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    rememberChannel(trimmed);
    m_redis.publish(trimmed, message);
    statusBar()->showMessage(QString("Python published to %1").arg(trimmed));
}

void MainWindow::openPythonEditor()
{
    QString path = m_pythonScript->text().trimmed();
    if (path.isEmpty()) {
        path = QFileDialog::getOpenFileName(this, "Python script", QString(), "Python files (*.py);;All files (*)");
        if (path.isEmpty()) {
            return;
        }
        m_pythonScript->setText(path);
    }

    auto *editor = new PythonEditorWindow(path);
    auto *subWindow = m_mdi->addSubWindow(editor);
    subWindow->setWindowTitle(QString("Edit %1").arg(QFileInfo(path).fileName()));
    subWindow->resize(760, 560);
    editor->show();
    m_mdi->setActiveSubWindow(subWindow);

    connect(editor, &PythonEditorWindow::saved, this, [this](const QString &savedPath) {
        m_pythonScript->setText(savedPath);
        loadPythonScript();
        statusBar()->showMessage(QString("Saved and loaded %1").arg(savedPath));
    });
}

void MainWindow::loadThemes()
{
    m_themeStyles.clear();
    m_themeMainWindowBackgrounds.clear();
    m_theme->clear();

    for (const QString &directoryPath : themeDirectories()) {
        const QDir directory(directoryPath);
        if (!directory.exists()) {
            continue;
        }

        const QStringList files = directory.entryList({"*.json"}, QDir::Files, QDir::Name);
        for (const QString &fileName : files) {
            QFile file(directory.filePath(fileName));
            if (!file.open(QIODevice::ReadOnly)) {
                continue;
            }

            QJsonParseError error;
            const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
            if (error.error != QJsonParseError::NoError || !document.isObject()) {
                continue;
            }

            const QJsonObject object = document.object();
            const QString name = object.value("name").toString();
            const QString stylesheet = object.value("stylesheet").toString();
            const QString mainWindowBackground = object.value("mainWindowBackground").toString();
            if (name.isEmpty() || m_themeStyles.contains(name)) {
                continue;
            }

            m_themeStyles.insert(name, stylesheet);
            if (!mainWindowBackground.isEmpty()) {
                m_themeMainWindowBackgrounds.insert(name, mainWindowBackground);
            }
            m_theme->addItem(name);
        }
    }

    if (m_themeStyles.isEmpty()) {
        m_themeStyles.insert("System", QString());
        m_theme->addItem("System");
    }
}

QStringList MainWindow::themeDirectories() const
{
    const QString appDirectory = QCoreApplication::applicationDirPath();
    QStringList directories;
    directories << QDir(appDirectory).filePath("themes");
    directories << QDir(QDir::currentPath()).filePath("themes");
    directories << QFileInfo(QDir(appDirectory).filePath("../themes")).absoluteFilePath();
    directories << QFileInfo(QDir(appDirectory).filePath("../../themes")).absoluteFilePath();
    directories.removeDuplicates();
    return directories;
}

void MainWindow::updateChannelWindowTitles()
{
    if (!m_mdi) {
        return;
    }

    QMdiSubWindow *active = m_mdi->activeSubWindow();
    for (ChannelWindow *window : std::as_const(m_windows)) {
        auto *subWindow = qobject_cast<QMdiSubWindow *>(window->parentWidget());
        if (!subWindow) {
            continue;
        }

        const QString channel = subWindow->property("channelName").toString();
        const bool isActive = subWindow == active;
        QFont titleFont = subWindow->font();
        titleFont.setPointSize(isActive ? 11 : 9);
        titleFont.setBold(isActive);
        subWindow->setFont(titleFont);
        subWindow->setWindowTitle(isActive ? QString("%1  [active]").arg(channel) : channel);

        if (isActive) {
            subWindow->setStyleSheet(R"(
                QMdiSubWindow {
                    color: #f6c177;
                    border: 2px solid #8fd7ff;
                    font-weight: 800;
                }
            )");
        } else {
            subWindow->setStyleSheet(R"(
                QMdiSubWindow {
                    color: #d7e6ff;
                    border: 1px solid #59677c;
                    font-weight: 500;
                }
            )");
        }
    }
}

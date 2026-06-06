#pragma once

#include "ChannelWindow.h"
#include "RedisConnection.h"
#include "SshTunnel.h"

#include <QHash>
#include <QMainWindow>

class QCheckBox;
class QCloseEvent;
class QComboBox;
class QLineEdit;
class QListWidget;
class QMdiArea;
class QSpinBox;
class QStatusBar;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void connectClicked();
    void disconnectClicked();
    void onConnected();
    void onChannelsReceived(const QStringList &channels);
    void openChannel(const QString &channel);
    void onSubscribed(const QString &channel);
    void onUnsubscribed(const QString &channel);
    void onMessage(const QString &channel, const QString &message);
    void applyTheme(const QString &theme);

private:
    QWidget *buildConnectionPanel();
    QWidget *buildChannelPanel();
    RedisConfig redisConfig() const;
    SshTunnelConfig tunnelConfig() const;
    void setUiConnected(bool connected);
    void loadSettings();
    void saveSettings() const;
    void loadConnectionProfiles();
    void loadConnectionProfile(const QString &name);
    void saveCurrentConnectionProfile();
    void deleteCurrentConnectionProfile();
    void resetConnectionFields();
    QString currentConnectionName() const;

    RedisConnection m_redis;
    SshTunnel m_tunnel;
    QMdiArea *m_mdi = nullptr;
    QListWidget *m_connectionList = nullptr;
    QLineEdit *m_connectionName = nullptr;
    QListWidget *m_channelList = nullptr;
    QLineEdit *m_manualChannel = nullptr;
    QComboBox *m_theme = nullptr;
    QLineEdit *m_host = nullptr;
    QSpinBox *m_port = nullptr;
    QLineEdit *m_username = nullptr;
    QLineEdit *m_password = nullptr;
    QSpinBox *m_database = nullptr;
    QCheckBox *m_useSsh = nullptr;
    QLineEdit *m_sshHost = nullptr;
    QSpinBox *m_sshPort = nullptr;
    QLineEdit *m_sshUser = nullptr;
    QLineEdit *m_identityFile = nullptr;
    QSpinBox *m_localPort = nullptr;
    QHash<QString, ChannelWindow *> m_windows;
};

#pragma once

#include "ChannelWindow.h"
#include "JsonTreeWindow.h"
#include "PythonClient.h"
#include "RedisConnection.h"
#include "SshTunnel.h"

#include <QColor>
#include <QFont>
#include <QHash>
#include <QMainWindow>
#include <QStringList>

class QCheckBox;
class QCloseEvent;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QMdiArea;
class QPushButton;
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
    void rememberChannel(const QString &channel);
    void updateChannelList();
    void chooseChannelFont();
    void chooseChannelTextColor();
    void applyChannelAppearance();
    void updateChannelAppearanceControls();
    void openJsonViewer(const QString &channel, const QString &jsonText);
    void loadPythonScript();
    void updateChannelWindowTitles();
    void loadThemes();
    QStringList themeDirectories() const;

    RedisConnection m_redis;
    SshTunnel m_tunnel;
    PythonClient m_python;
    QMdiArea *m_mdi = nullptr;
    QListWidget *m_connectionList = nullptr;
    QLineEdit *m_connectionName = nullptr;
    QListWidget *m_channelList = nullptr;
    QLineEdit *m_manualChannel = nullptr;
    QPushButton *m_channelFontButton = nullptr;
    QPushButton *m_channelTextColorButton = nullptr;
    QLabel *m_channelTextColorPreview = nullptr;
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
    QCheckBox *m_pythonEnabled = nullptr;
    QLineEdit *m_pythonScript = nullptr;
    QStringList m_liveChannels;
    QStringList m_recentChannels;
    QFont m_channelFont;
    QColor m_channelTextColor = QColor("#202124");
    QHash<QString, QString> m_themeStyles;
    QHash<QString, ChannelWindow *> m_windows;
    int m_jsonWindowCount = 0;
};

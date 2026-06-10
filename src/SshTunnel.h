#pragma once

#include <QObject>
#include <QProcess>
#include <QTcpSocket>
#include <QTimer>

struct SshTunnelConfig {
    bool enabled = false;
    QString sshHost;
    quint16 sshPort = 22;
    QString sshUser;
    QString identityFile;
    QString remoteHost = "127.0.0.1";
    quint16 remotePort = 6379;
    quint16 localPort = 16379;
};

class SshTunnel : public QObject {
    Q_OBJECT

public:
    explicit SshTunnel(QObject *parent = nullptr);
    ~SshTunnel() override;

    void start(const SshTunnelConfig &config);
    void stop();
    bool isRunning() const;

signals:
    void ready();
    void stopped();
    void errorOccurred(const QString &message);

private slots:
    void onStarted();
    void onError(QProcess::ProcessError error);
    void onFinished(int exitCode, QProcess::ExitStatus status);
    void probe();

private:
    QProcess m_process;
    QTimer m_probeTimer;
    QTcpSocket m_probeSocket;
    quint16 m_localPort = 0;
    int m_probeAttempts = 0;
    static constexpr int k_maxProbeAttempts = 100; // 10 s total
};

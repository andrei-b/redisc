#include "SshTunnel.h"

SshTunnel::SshTunnel(QObject *parent)
    : QObject(parent)
{
    connect(&m_process, &QProcess::started,        this, &SshTunnel::onStarted);
    connect(&m_process, &QProcess::errorOccurred,  this, &SshTunnel::onError);
    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &SshTunnel::onFinished);

    m_probeTimer.setInterval(100);
    connect(&m_probeTimer, &QTimer::timeout, this, &SshTunnel::probe);

    // Probe: declare connected/refused immediately, don't wait for full handshake.
    connect(&m_probeSocket, &QTcpSocket::connected, this, [this]() {
        m_probeSocket.abort();
        m_probeTimer.stop();
        emit ready();
    });
    connect(&m_probeSocket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        m_probeSocket.abort();
        // Will retry on next timer tick (or give up in probe()).
    });
}

SshTunnel::~SshTunnel()
{
    stop();
}

void SshTunnel::start(const SshTunnelConfig &config)
{
    stop();

    m_localPort     = config.localPort;
    m_probeAttempts = 0;

    QStringList args;
    args << "-N"
         << "-o" << "ExitOnForwardFailure=yes"
         << "-o" << "ServerAliveInterval=30"
         << "-o" << "BatchMode=yes"
         << "-p" << QString::number(config.sshPort)
         << "-L" << QString("%1:%2:%3")
                    .arg(config.localPort)
                    .arg(config.remoteHost)
                    .arg(config.remotePort);
    if (!config.identityFile.trimmed().isEmpty()) {
        args << "-i" << config.identityFile.trimmed();
    }
    args << (config.sshUser.trimmed().isEmpty()
                 ? config.sshHost.trimmed()
                 : QString("%1@%2").arg(config.sshUser.trimmed(), config.sshHost.trimmed()));

    m_process.setProgram("ssh");
    m_process.setArguments(args);
    m_process.start();
}

void SshTunnel::stop()
{
    m_probeTimer.stop();
    m_probeSocket.abort();

    if (m_process.state() == QProcess::NotRunning) {
        return;
    }
    m_process.terminate();
    if (!m_process.waitForFinished(1500)) {
        m_process.kill();
    }
}

bool SshTunnel::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

void SshTunnel::onStarted()
{
    m_probeAttempts = 0;
    m_probeTimer.start();
}

void SshTunnel::probe()
{
    if (!isRunning()) {
        // SSH process died before tunnel was ready; onFinished will emit stopped().
        m_probeTimer.stop();
        return;
    }
    if (m_probeAttempts >= k_maxProbeAttempts) {
        m_probeTimer.stop();
        emit errorOccurred("SSH tunnel did not become ready within 10 seconds.");
        return;
    }
    ++m_probeAttempts;
    if (m_probeSocket.state() == QAbstractSocket::UnconnectedState) {
        m_probeSocket.connectToHost(QHostAddress::LocalHost, m_localPort);
    }
}

void SshTunnel::onError(QProcess::ProcessError)
{
    m_probeTimer.stop();
    m_probeSocket.abort();
    emit errorOccurred(m_process.errorString());
}

void SshTunnel::onFinished(int exitCode, QProcess::ExitStatus)
{
    m_probeTimer.stop();
    m_probeSocket.abort();

    if (exitCode != 0) {
        const QString stderrText = QString::fromLocal8Bit(m_process.readAllStandardError()).trimmed();
        if (!stderrText.isEmpty()) {
            emit errorOccurred(stderrText);
        }
    }
    emit stopped();
}

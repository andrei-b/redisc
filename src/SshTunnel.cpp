#include "SshTunnel.h"

#include <QTimer>

SshTunnel::SshTunnel(QObject *parent)
    : QObject(parent)
{
    connect(&m_process, &QProcess::started, this, &SshTunnel::onStarted);
    connect(&m_process, &QProcess::errorOccurred, this, &SshTunnel::onError);
    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &SshTunnel::onFinished);
}

SshTunnel::~SshTunnel()
{
    stop();
}

void SshTunnel::start(const SshTunnelConfig &config)
{
    stop();
    QStringList args;
    args << "-N"
         << "-o" << "ExitOnForwardFailure=yes"
         << "-o" << "ServerAliveInterval=30"
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
    QTimer::singleShot(350, this, &SshTunnel::ready);
}

void SshTunnel::onError(QProcess::ProcessError)
{
    emit errorOccurred(m_process.errorString());
}

void SshTunnel::onFinished(int exitCode, QProcess::ExitStatus)
{
    if (exitCode != 0) {
        const QString stderrText = QString::fromLocal8Bit(m_process.readAllStandardError()).trimmed();
        if (!stderrText.isEmpty()) {
            emit errorOccurred(stderrText);
        }
    }
    emit stopped();
}

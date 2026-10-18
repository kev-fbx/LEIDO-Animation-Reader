#pragma once

#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class AgentClient;
class AppSettings;

// Starts ProjectZomboid64.exe with the agent attached through JAVA_TOOL_OPTIONS (same approach as
// launcher/Launch-PZ-With-Agent.ps1); no game files are modified.
class GameLauncher : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created in main.cpp")

    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool busy READ isBusy NOTIFY statusChanged)

public:
    GameLauncher(AppSettings *settings, AgentClient *agent, QObject *parent = nullptr);

    QString status() const { return m_status; }
    bool isBusy() const { return m_waiting.isActive(); }

    Q_INVOKABLE void launch();

signals:
    void statusChanged();

private:
    QString findAgentJar() const;
    void setStatus(const QString &status);

    AppSettings *m_settings;
    AgentClient *m_agent;
    QTimer m_waiting;
    QString m_status;
};

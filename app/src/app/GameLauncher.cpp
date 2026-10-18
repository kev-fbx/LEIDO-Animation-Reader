#include "app/GameLauncher.h"

#include "app/AppSettings.h"
#include "net/AgentClient.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QProcess>

#include <windows.h>
#include <tlhelp32.h>

Q_LOGGING_CATEGORY(lcLaunch, "pz.launch")

namespace {

bool isProcessRunning(const wchar_t *exeName)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return false;
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    bool found = false;
    for (BOOL ok = Process32FirstW(snapshot, &entry); ok && !found; ok = Process32NextW(snapshot, &entry))
        found = _wcsicmp(entry.szExeFile, exeName) == 0;
    CloseHandle(snapshot);
    return found;
}

} // namespace

GameLauncher::GameLauncher(AppSettings *settings, AgentClient *agent, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_agent(agent)
{
    // The game takes a while to boot its JVM; give the agent a minute to answer.
    m_waiting.setSingleShot(true);
    m_waiting.setInterval(60 * 1000);
    connect(&m_waiting, &QTimer::timeout, this, [this] {
        setStatus(isProcessRunning(L"ProjectZomboid64.exe")
                      ? "The game is running but the agent did not answer. See runtime\\agent.log."
                      : "The game exited during start-up. See %USERPROFILE%\\Zomboid\\console.txt.");
    });
    connect(m_agent, &AgentClient::connectedChanged, this, [this] {
        if (m_agent->isConnected() && m_waiting.isActive()) {
            m_waiting.stop();
            setStatus({});
        }
    });
}

void GameLauncher::launch()
{
    const QString gameDir = m_settings->pzInstallDir();
    const QString exe = QDir(gameDir).filePath("ProjectZomboid64.exe");
    const QString agentJar = findAgentJar();
    // Agent log lives in <repo>/runtime like the PowerShell launcher; takes go wherever Record says.
    const QString agentHome = agentJar.isEmpty() ? QString()
                                                 : QDir::cleanPath(QFileInfo(agentJar).dir().filePath("../../runtime"));

    if (!QFileInfo::exists(exe))
        return setStatus("Project Zomboid not found in " + QDir::toNativeSeparators(gameDir));
    if (agentJar.isEmpty())
        return setStatus("Agent not built. Run agent\\build.ps1 first.");
    if (isProcessRunning(L"ProjectZomboid64.exe"))
        return setStatus("Project Zomboid is already running. Quit it, then launch from here.");
    if (!isProcessRunning(L"steam.exe"))
        return setStatus("Start Steam first.");
    // JAVA_TOOL_OPTIONS splits on spaces, so the agent's paths must not contain any.
    if (agentJar.contains(' ') || agentHome.contains(' '))
        return setStatus("Agent or output path contains spaces, which JAVA_TOOL_OPTIONS cannot carry: "
                         + QDir::toNativeSeparators(agentJar.contains(' ') ? agentJar : agentHome));

    QDir().mkpath(agentHome);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    // instrument.dll (the JVM's agent support) imports jli.dll, which only exists in jre64\bin.
    // java.exe finds it next to itself; ProjectZomboid64.exe sits one folder up, so without this
    // the JVM aborts with "Could not find agent library instrument ... Can't find dependent libraries".
    env.insert("PATH", QDir::toNativeSeparators(QDir(gameDir).filePath("jre64/bin")) + ';' + env.value("PATH"));
    env.insert("JAVA_TOOL_OPTIONS", QStringLiteral("-javaagent:%1=port=%2,home=%3")
                                        .arg(QDir::toNativeSeparators(agentJar))
                                        .arg(m_agent->port())
                                        .arg(QDir::toNativeSeparators(agentHome)));

    QProcess process;
    process.setProgram(exe);
    process.setWorkingDirectory(gameDir);
    process.setProcessEnvironment(env);
    qint64 pid = 0;
    if (!process.startDetached(&pid))
        return setStatus("Could not start ProjectZomboid64.exe: " + process.errorString());

    qCInfo(lcLaunch) << "Started PZ, pid" << pid << "agent" << agentJar;
    setStatus("Starting Project Zomboid...");
    m_waiting.start();
}

QString GameLauncher::findAgentJar() const
{
    // The exe lives in <repo>/app/build/<Config>/; look upwards for agent/build/pz-animator-agent.jar.
    QDir dir(QCoreApplication::applicationDirPath());
    for (int depth = 0; depth < 5; ++depth) {
        const QString candidate = dir.filePath("agent/build/pz-animator-agent.jar");
        if (QFileInfo::exists(candidate))
            return QDir::cleanPath(candidate);
        if (!dir.cdUp())
            break;
    }
    return {};
}

void GameLauncher::setStatus(const QString &status)
{
    if (!status.isEmpty())
        qCInfo(lcLaunch).noquote() << status;
    m_status = status;
    emit statusChanged();
}

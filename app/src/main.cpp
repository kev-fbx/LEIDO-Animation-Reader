#include "app/AppSettings.h"
#include "app/GameLauncher.h"
#include "app/TakeExporter.h"
#include "app/TakeLibrary.h"
#include "assets/PzAssets.h"
#include "net/AgentClient.h"
#include "platform/GlobalHotkey.h"
#include "scene/PoseState.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QMutex>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QStandardPaths>
#include <QtQuick3D/QQuick3D>
#include <QTimer>

#include <windows.h>

namespace {

QFile *g_logFile = nullptr;
QtMessageHandler g_previousHandler = nullptr;
const QString name = "LEIDO PZ Animation Exporter";

// A GUI-subsystem exe has no console, so mirror all Qt logging to app.log.
void logToFile(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    static QMutex mutex;
    {
        QMutexLocker lock(&mutex);
        if (g_logFile) {
            static const char *levels[] = {"DEBUG", "WARN ", "CRIT ", "FATAL", "INFO "};
            const QString line = QStringLiteral("%1 %2 %3: %4\n")
                                     .arg(QDateTime::currentDateTime().toString("HH:mm:ss.zzz"),
                                          QString::fromLatin1(levels[type]),
                                          QString::fromLatin1(context.category ? context.category : "default"),
                                          message);
            g_logFile->write(line.toUtf8());
            g_logFile->flush();
        }
    }
    if (g_previousHandler)
        g_previousHandler(type, context, message);
}

void installFileLog()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dir);
    g_logFile = new QFile(QDir(dir).filePath("app.log"));
    if (!g_logFile->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        delete g_logFile;
        g_logFile = nullptr;
        return;
    }
    g_previousHandler = qInstallMessageHandler(logToFile);
}

} // namespace

int main(int argc, char *argv[])
{
    QSurfaceFormat::setDefaultFormat(QQuick3D::idealSurfaceFormat());
    // The Windows 7 look is drawn by our own Aero* controls; "Basic" is the style meant for that.
    QQuickStyle::setStyle("Basic");
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(name);
    QGuiApplication::setOrganizationName(name);
    installFileLog();
    qInfo() << name << PROJECT_VERSION << "starting";

    AppSettings settings;
    AgentClient agent;
    PoseState pose;
    PzAssets assets(settings.pzInstallDir());
    GameLauncher launcher(&settings, &agent);
    TakeExporter exporter(&agent, &settings, &assets);
    TakeLibrary library(&settings);
    GlobalHotkey recordHotkey(VK_F9);

    QObject::connect(&agent, &AgentClient::skeletonReceived, &pose, &PoseState::setSkeleton);
    QObject::connect(&agent, &AgentClient::skeletonReceived, &assets, &PzAssets::compareWithSkeleton);
    QObject::connect(&agent, &AgentClient::frameReceived, &pose, &PoseState::setFrame);
    QObject::connect(&recordHotkey, &GlobalHotkey::activated, &agent,
                     [&] { agent.toggleRecording(settings.rawTakesDir()); });

    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {"agent", QVariant::fromValue(&agent)},
        {"pose", QVariant::fromValue(&pose)},
        {"assets", QVariant::fromValue(&assets)},
        {"settings", QVariant::fromValue(&settings)},
        {"launcher", QVariant::fromValue(&launcher)},
        {"exporter", QVariant::fromValue(&exporter)},
        {"library", QVariant::fromValue(&library)},
        {"hotkeyReady", recordHotkey.isRegistered()},
    });
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("PzAnimator", "Main"); // the QML module URI in CMakeLists.txt, not the display name

    // Parse the body mesh after the window is up.
    QTimer::singleShot(0, &assets, &PzAssets::probeBody);
    // "PzAnimator.exe --launch" starts the game with the agent as soon as the window is open.
    if (QCoreApplication::arguments().contains("--launch"))
        QTimer::singleShot(0, &launcher, &GameLauncher::launch);

    return app.exec();
}

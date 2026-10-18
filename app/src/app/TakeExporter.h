#pragma once

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class AgentClient;
class AppSettings;
class PzAssets;

// Exports .pzrec takes to GLB in the output folder, one at a time on a worker thread.
// Also exports each new take automatically when recording stops (if enabled in settings).
class TakeExporter : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created in main.cpp")

    Q_PROPERTY(bool busy READ isBusy NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString lastGlb READ lastGlb NOTIFY changed)
    Q_PROPERTY(int done READ done NOTIFY changed)
    Q_PROPERTY(int total READ total NOTIFY changed)

public:
    TakeExporter(AgentClient *agent, AppSettings *settings, PzAssets *assets, QObject *parent = nullptr);

    bool isBusy() const { return m_current; }
    QString status() const { return m_status; }
    QString lastGlb() const { return m_lastGlb; }
    int done() const { return m_done; }
    int total() const { return m_total; }

    Q_INVOKABLE void exportTakes(const QStringList &takePaths);
    Q_INVOKABLE void exportUrls(const QList<QUrl> &takeUrls);

signals:
    void changed();
    void exported(const QString &glbPath);

private:
    void onStatusChanged();
    void startNext();

    AgentClient *m_agent;
    AppSettings *m_settings;
    PzAssets *m_assets;
    bool m_wasRecording = false;
    bool m_current = false;
    QStringList m_queue;
    int m_done = 0;
    int m_total = 0;
    int m_failed = 0;
    QString m_status;
    QString m_lastGlb;
};

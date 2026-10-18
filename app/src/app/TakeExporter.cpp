#include "app/TakeExporter.h"

#include "app/AppSettings.h"
#include "assets/PzAssets.h"
#include "export/GlbExporter.h"
#include "export/TakeFile.h"
#include "net/AgentClient.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QPointer>
#include <QThread>

Q_LOGGING_CATEGORY(lcExport, "pz.export")

TakeExporter::TakeExporter(AgentClient *agent, AppSettings *settings, PzAssets *assets, QObject *parent)
    : QObject(parent)
    , m_agent(agent)
    , m_settings(settings)
    , m_assets(assets)
{
    connect(m_agent, &AgentClient::statusChanged, this, &TakeExporter::onStatusChanged);
}

void TakeExporter::onStatusChanged()
{
    const bool recording = m_agent->isRecording();
    const bool justStopped = m_wasRecording && !recording;
    m_wasRecording = recording;
    if (justStopped && m_settings->autoExport() && !m_agent->lastTake().isEmpty())
        exportTakes({m_agent->lastTake()});
}

void TakeExporter::exportUrls(const QList<QUrl> &takeUrls)
{
    QStringList paths;
    for (const QUrl &url : takeUrls)
        paths << url.toLocalFile();
    exportTakes(paths);
}

void TakeExporter::exportTakes(const QStringList &takePaths)
{
    for (const QString &path : takePaths) {
        if (!path.isEmpty() && !m_queue.contains(path)) {
            m_queue << path;
            ++m_total;
        }
    }
    if (!m_current)
        startNext();
    emit changed();
}

void TakeExporter::startNext()
{
    if (m_queue.isEmpty()) {
        m_current = false;
        if (m_total > 0) {
            m_status = m_failed == 0
                ? QStringLiteral("Exported %1 take%2 to %3").arg(m_done).arg(m_done == 1 ? "" : "s")
                      .arg(QDir::toNativeSeparators(m_settings->outputDir()))
                : QStringLiteral("Exported %1 of %2 takes; %3 failed (see the log)")
                      .arg(m_done - m_failed).arg(m_total).arg(m_failed);
        }
        m_done = m_total = m_failed = 0;
        emit changed();
        return;
    }

    m_current = true;
    const QString takePath = m_queue.takeFirst();
    const QString outPath = QDir(m_settings->outputDir()).filePath(QFileInfo(takePath).completeBaseName() + ".glb");
    m_status = QStringLiteral("Exporting %1 (%2 of %3)...")
                   .arg(QFileInfo(takePath).completeBaseName()).arg(m_done + 1).arg(m_total);
    emit changed();

    const auto body = m_assets->body(); // shared, immutable
    ExportOptions options;
    options.fps = m_settings->exportFps();
    options.worldSpaceRoot = m_settings->exportWorldRoot();
    options.metresPerModelUnit = float(m_settings->exportScale());

    QPointer<TakeExporter> self(this);
    QThread *worker = QThread::create([self, takePath, outPath, body, options] {
        bool ok = false;
        QString error;
        if (auto take = readTake(takePath, &error)) {
            const ExportResult result = exportGlb(*take, body.get(), options, outPath);
            ok = result.ok;
            error = result.message;
            qCInfo(lcExport).noquote() << result.message << "| self-check error"
                                       << result.selfCheckErrorMetres * 1000 << "mm";
        } else {
            qCWarning(lcExport).noquote() << error;
        }
        QMetaObject::invokeMethod(self, [self, ok, outPath] {
            if (!self)
                return;
            ++self->m_done;
            if (ok) {
                self->m_lastGlb = outPath;
                emit self->exported(outPath);
            } else {
                ++self->m_failed;
            }
            self->startNext();
        });
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

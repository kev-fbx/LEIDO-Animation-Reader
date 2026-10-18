#include "app/AppSettings.h"

#include <QDesktopServices>
#include <QDir>
#include <QStandardPaths>

namespace {
constexpr auto kOutputDir = "output/dir";
constexpr auto kPzInstallDir = "game/installDir";
constexpr auto kDefaultPzInstall = "C:/Program Files (x86)/Steam/steamapps/common/ProjectZomboid";
constexpr auto kExportFps = "export/fps";
constexpr auto kExportWorldRoot = "export/worldRoot";
constexpr auto kExportScale = "export/metresPerModelUnit";
constexpr auto kAutoExport = "export/auto";
constexpr auto kAppName = "LEIDO PZ Animation Exporter";
constexpr auto kLegacyAppName = "PZ Animator";
} // namespace

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
    , m_settings(QSettings::NativeFormat, QSettings::UserScope, kAppName, kAppName)
{
    // One-time copy of settings saved before the rename.
    if (m_settings.allKeys().isEmpty()) {
        const QSettings legacy(QSettings::NativeFormat, QSettings::UserScope, kLegacyAppName, kLegacyAppName);
        for (const QString &key : legacy.allKeys())
            m_settings.setValue(key, legacy.value(key));
    }
}

QString AppSettings::outputDir() const
{
    // Keep using the pre-rename takes folder if it already exists.
    const QDir documents(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    const QString legacy = documents.filePath(kLegacyAppName);
    const QString fallback = QDir(legacy).exists() ? legacy : documents.filePath(kAppName);
    return m_settings.value(kOutputDir, fallback).toString();
}

void AppSettings::setOutputDir(const QString &dir)
{
    if (dir.isEmpty() || dir == outputDir())
        return;
    m_settings.setValue(kOutputDir, QDir::cleanPath(dir));
    emit outputDirChanged();
}

void AppSettings::setOutputDirUrl(const QUrl &url)
{
    setOutputDir(url.toLocalFile());
}

QString AppSettings::rawTakesDir() const
{
    return QDir(outputDir()).filePath("raw");
}

QString AppSettings::pzInstallDir() const
{
    return m_settings.value(kPzInstallDir, kDefaultPzInstall).toString();
}

double AppSettings::exportFps() const
{
    return m_settings.value(kExportFps, 30.0).toDouble();
}

void AppSettings::setExportFps(double fps)
{
    if (fps > 0 && fps != exportFps())
        setValue(kExportFps, fps);
}

bool AppSettings::exportWorldRoot() const
{
    return m_settings.value(kExportWorldRoot, true).toBool();
}

void AppSettings::setExportWorldRoot(bool world)
{
    if (world != exportWorldRoot())
        setValue(kExportWorldRoot, world);
}

double AppSettings::exportScale() const
{
    return m_settings.value(kExportScale, 2.0).toDouble();
}

void AppSettings::setExportScale(double scale)
{
    if (scale > 0 && !qFuzzyCompare(scale, exportScale()))
        setValue(kExportScale, scale);
}

bool AppSettings::autoExport() const
{
    return m_settings.value(kAutoExport, true).toBool();
}

void AppSettings::setAutoExport(bool enabled)
{
    if (enabled != autoExport())
        setValue(kAutoExport, enabled);
}

void AppSettings::openFolder(const QString &dir) const
{
    QDir().mkpath(dir);
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void AppSettings::setValue(const char *key, const QVariant &value)
{
    m_settings.setValue(key, value);
    emit exportOptionsChanged();
}

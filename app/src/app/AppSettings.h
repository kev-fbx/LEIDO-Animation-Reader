#pragma once

#include <QObject>
#include <QSettings>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

// Persistent user settings (stored by QSettings under HKCU\Software\LEIDO PZ Animation Exporter;
// settings saved under the old "PZ Animator" name are copied over on first run).
class AppSettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created in main.cpp")

    Q_PROPERTY(QString outputDir READ outputDir WRITE setOutputDir NOTIFY outputDirChanged)
    Q_PROPERTY(QString rawTakesDir READ rawTakesDir NOTIFY outputDirChanged)
    Q_PROPERTY(QString pzInstallDir READ pzInstallDir CONSTANT)
    Q_PROPERTY(double exportFps READ exportFps WRITE setExportFps NOTIFY exportOptionsChanged)
    Q_PROPERTY(bool exportWorldRoot READ exportWorldRoot WRITE setExportWorldRoot NOTIFY exportOptionsChanged)
    Q_PROPERTY(double exportScale READ exportScale WRITE setExportScale NOTIFY exportOptionsChanged)
    Q_PROPERTY(bool autoExport READ autoExport WRITE setAutoExport NOTIFY exportOptionsChanged)

public:
    explicit AppSettings(QObject *parent = nullptr);

    QString outputDir() const;
    void setOutputDir(const QString &dir);
    Q_INVOKABLE void setOutputDirUrl(const QUrl &url);

    // Raw agent takes go to <output>/raw; exported GLBs sit in <output>.
    QString rawTakesDir() const;
    QString pzInstallDir() const;

    double exportFps() const;
    void setExportFps(double fps);
    bool exportWorldRoot() const;
    void setExportWorldRoot(bool world);
    // Metres per PZ model unit (2.0 makes the character about 1.96 m tall).
    double exportScale() const;
    void setExportScale(double scale);
    // Export a GLB automatically when recording stops.
    bool autoExport() const;
    void setAutoExport(bool enabled);

    Q_INVOKABLE void openFolder(const QString &dir) const;

signals:
    void outputDirChanged();
    void exportOptionsChanged();

private:
    void setValue(const char *key, const QVariant &value);

    QSettings m_settings;
};

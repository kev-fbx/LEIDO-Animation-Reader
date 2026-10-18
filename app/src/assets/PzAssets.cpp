#include "assets/PzAssets.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>

#include <algorithm>

Q_LOGGING_CATEGORY(lcAssets, "pz.assets")

PzAssets::PzAssets(QString installDir, QObject *parent)
    : QObject(parent)
    , m_installDir(std::move(installDir))
{
}

QString PzAssets::bodyMeshPath(bool female) const
{
    return QDir(m_installDir).filePath(female ? "media/models_X/Skinned/FemaleBody.x"
                                              : "media/models_X/Skinned/MaleBody.x");
}

void PzAssets::probeBody()
{
    QString error;
    auto mesh = loadBodyMesh(bodyMeshPath(), m_installDir, &error);
    if (!mesh) {
        setReport(error);
        return;
    }
    m_meshSummary = QStringLiteral("MaleBody.x: %1 vertices, %2 weighted bones, texture %3")
                        .arg(mesh->vertices.size())
                        .arg(mesh->boneNames.size())
                        .arg(mesh->texturePath.isEmpty() ? QStringLiteral("not found")
                                                         : QFileInfo(mesh->texturePath).fileName());
    m_body = std::make_shared<const BodyMesh>(std::move(*mesh));
    setReport(m_meshSummary);
}

void PzAssets::compareWithSkeleton(const pz::Skeleton &skeleton)
{
    if (!m_body)
        return;
    const auto &names = m_body->boneNames;
    const auto missing = std::count_if(names.cbegin(), names.cend(),
                                       [&](const QString &name) { return skeleton.indexOf(name) < 0; });
    const QString match = missing == 0
        ? QStringLiteral("all %1 mesh bones found in the game skeleton").arg(names.size())
        : QStringLiteral("%1 of %2 mesh bones missing from the game skeleton").arg(missing).arg(names.size());
    setReport(m_meshSummary + QStringLiteral(" - ") + match);
}

void PzAssets::setReport(const QString &report)
{
    if (m_report == report)
        return;
    qCInfo(lcAssets).noquote() << report;
    m_report = report;
    emit reportChanged();
}

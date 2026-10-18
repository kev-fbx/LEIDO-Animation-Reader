#pragma once

#include "export/BodyMesh.h"
#include "protocol/Protocol.h"

#include <QObject>
#include <QtQml/qqmlregistration.h>

#include <memory>

// Loads PZ model files from the local install and checks them against the streamed skeleton.
class PzAssets : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created in main.cpp")

    Q_PROPERTY(QString report READ report NOTIFY reportChanged)

public:
    explicit PzAssets(QString installDir, QObject *parent = nullptr);

    QString report() const { return m_report; }
    QString bodyMeshPath(bool female = false) const;
    // Parsed body mesh, or null if it could not be loaded. Immutable and safe to share across threads.
    std::shared_ptr<const BodyMesh> body() const { return m_body; }

public slots:
    void probeBody();
    void compareWithSkeleton(const pz::Skeleton &skeleton);

signals:
    void reportChanged();

private:
    void setReport(const QString &report);

    QString m_installDir;
    std::shared_ptr<const BodyMesh> m_body;
    QString m_meshSummary;
    QString m_report;
};

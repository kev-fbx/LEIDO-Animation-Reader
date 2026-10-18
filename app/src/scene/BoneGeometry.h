#pragma once

#include "scene/PoseState.h"

#include <QPointer>
#include <QtQuick3D/QQuick3DGeometry>
#include <QtQml/qqmlregistration.h>

// Draws the streamed skeleton as line segments. Placeholder until the skinned PZ body mesh lands.
class BoneGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(PoseState *pose READ pose WRITE setPose NOTIFY poseChanged)

public:
    explicit BoneGeometry(QQuick3DObject *parent = nullptr);

    PoseState *pose() const { return m_pose; }
    void setPose(PoseState *pose);

signals:
    void poseChanged();

private:
    void rebuild();

    QPointer<PoseState> m_pose;
    QMetaObject::Connection m_poseConnection;
};

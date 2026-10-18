#pragma once

#include "protocol/Protocol.h"

#include <QObject>
#include <QVector3D>
#include <QtQml/qqmlregistration.h>

// Latest skeleton and pose from the agent, converted to viewport scene space.
class PoseState : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created in main.cpp")

    Q_PROPERTY(bool hasPose READ hasPose NOTIFY poseChanged)
    Q_PROPERTY(QVector3D focus READ focus NOTIFY poseChanged)
    Q_PROPERTY(int boneCount READ boneCount NOTIFY skeletonChanged)

public:
    using QObject::QObject;

    bool hasPose() const { return !m_joints.isEmpty(); }
    QVector3D focus() const { return m_focus; }
    int boneCount() const { return int(m_skeleton.bones.size()); }

    const pz::Skeleton &skeleton() const { return m_skeleton; }
    const QList<QVector3D> &joints() const { return m_joints; }
    // Pairs of joint indices to draw as bone lines.
    const QList<QPair<int, int>> &segments() const { return m_segments; }

public slots:
    void setSkeleton(const pz::Skeleton &skeleton);
    void setFrame(const pz::Frame &frame);

signals:
    void skeletonChanged();
    void poseChanged();

private:
    pz::Skeleton m_skeleton;
    QList<QPair<int, int>> m_segments;
    QList<QVector3D> m_joints;
    QVector3D m_focus;
    int m_focusBone = -1;
};

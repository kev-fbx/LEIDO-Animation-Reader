#include "scene/BoneGeometry.h"

#include <QVector3D>

BoneGeometry::BoneGeometry(QQuick3DObject *parent)
    : QQuick3DGeometry(parent)
{
    setPrimitiveType(PrimitiveType::Lines);
    setStride(sizeof(float) * 3);
    addAttribute(Attribute::PositionSemantic, 0, Attribute::F32Type);
}

void BoneGeometry::setPose(PoseState *pose)
{
    if (m_pose == pose)
        return;
    disconnect(m_poseConnection);
    m_pose = pose;
    if (m_pose)
        m_poseConnection = connect(m_pose, &PoseState::poseChanged, this, &BoneGeometry::rebuild);
    emit poseChanged();
    rebuild();
}

void BoneGeometry::rebuild()
{
    QByteArray vertices;
    QVector3D minimum(0, 0, 0);
    QVector3D maximum(0, 0, 0);

    if (m_pose && m_pose->hasPose()) {
        const auto &joints = m_pose->joints();
        const auto &segments = m_pose->segments();
        vertices.resize(segments.size() * 2 * 3 * qsizetype(sizeof(float)));
        auto *out = reinterpret_cast<float *>(vertices.data());
        minimum = maximum = joints.first();
        for (const auto &[from, to] : segments) {
            for (int index : {from, to}) {
                const QVector3D &p = joints[index];
                *out++ = p.x();
                *out++ = p.y();
                *out++ = p.z();
                minimum = QVector3D(qMin(minimum.x(), p.x()), qMin(minimum.y(), p.y()), qMin(minimum.z(), p.z()));
                maximum = QVector3D(qMax(maximum.x(), p.x()), qMax(maximum.y(), p.y()), qMax(maximum.z(), p.z()));
            }
        }
    }

    setVertexData(vertices);
    setBounds(minimum, maximum);
    update();
}

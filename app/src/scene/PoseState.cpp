#include "scene/PoseState.h"

#include "scene/PzSpace.h"

namespace {

// Helper bones that are not part of the visible body.
bool isHelperBone(const QString &name)
{
    return name.compare("Dummy01", Qt::CaseInsensitive) == 0
        || name.compare("Translation_Data", Qt::CaseInsensitive) == 0;
}

} // namespace

void PoseState::setSkeleton(const pz::Skeleton &skeleton)
{
    m_skeleton = skeleton;
    m_segments.clear();
    for (int i = 0; i < m_skeleton.bones.size(); ++i) {
        const int parent = m_skeleton.bones[i].parent;
        if (parent < 0 || isHelperBone(m_skeleton.bones[i].name) || isHelperBone(m_skeleton.bones[parent].name))
            continue;
        m_segments.append({parent, i});
    }
    m_focusBone = m_skeleton.indexOf("Bip01_Spine1");
    m_joints.clear();
    emit skeletonChanged();
    emit poseChanged();
}

void PoseState::setFrame(const pz::Frame &frame)
{
    if (frame.skeletonId != m_skeleton.id || frame.bones.size() != m_skeleton.bones.size())
        return; // Frames for a skeleton we have not received yet.

    m_joints.resize(frame.bones.size());
    for (qsizetype i = 0; i < frame.bones.size(); ++i) {
        const QVector3D world = pz::space::modelToWorld(pz::translationOf(frame.bones[i]), frame.renderedAngle,
                                                        frame.position);
        m_joints[i] = pz::space::worldToScene(world);
    }
    m_focus = m_focusBone >= 0 ? m_joints[m_focusBone] : pz::space::worldToScene(frame.position);
    emit poseChanged();
}

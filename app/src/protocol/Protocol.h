#pragma once

// Wire protocol v1 shared with the in-game agent. Source of truth: docs/protocol.md.

#include <QByteArray>
#include <QList>
#include <QString>
#include <QVector3D>

#include <array>
#include <optional>

namespace pz {

constexpr int kProtocolVersion = 1;
constexpr quint16 kDefaultPort = 47400;
constexpr quint32 kMaxMessageBytes = 16 * 1024 * 1024;

enum class MsgType : quint8 {
    Hello = 0x01,
    Skeleton = 0x02,
    Frame = 0x03,
    Status = 0x04,
    RecordStart = 0x81,
    RecordStop = 0x82,
    Ping = 0x83,
};

// The 16 public fields of org.lwjgl.util.vector.Matrix4f in order m00, m01, ... m33.
// PZ keeps translation in m03, m13, m23 and composes model = local * parent.
using Matrix = std::array<float, 16>;

inline QVector3D translationOf(const Matrix &m)
{
    return {m[3], m[7], m[11]};
}

struct Bone {
    QString name;
    int parent = -1;
};

struct Skeleton {
    quint32 id = 0;
    QList<Bone> bones;
    QList<Matrix> bindPose;
    QList<std::optional<Matrix>> boneOffset;

    int indexOf(const QString &boneName) const;
    static std::optional<Skeleton> fromJson(const QByteArray &json);
};

struct Frame {
    quint64 seq = 0;
    qint64 monoNanos = 0;
    float animDelta = 0;
    QVector3D position; // PZ world: x, y in tiles, z in levels
    float renderedAngle = 0;
    quint32 skeletonId = 0;
    bool recording = false;
    QList<Matrix> bones; // model space, one per skeleton bone

    static std::optional<Frame> parse(const QByteArray &payload);
};

struct Hello {
    int protocol = 0;
    QString agentVersion;
    QString gameVersion;
    QString revision;
    bool supported = false;

    static std::optional<Hello> fromJson(const QByteArray &json);
};

QByteArray encodeMessage(MsgType type, const QByteArray &payload = {});

} // namespace pz

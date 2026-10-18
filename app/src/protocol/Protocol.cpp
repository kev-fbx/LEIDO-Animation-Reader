#include "protocol/Protocol.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtEndian>

#include <cstring>

namespace pz {
namespace {

std::optional<Matrix> matrixFromJson(const QJsonValue &value)
{
    const QJsonArray values = value.toArray();
    if (values.size() != 16)
        return std::nullopt;
    Matrix m{};
    for (int i = 0; i < 16; ++i)
        m[i] = float(values[i].toDouble());
    return m;
}

template <typename T>
T readLE(const char *&cursor)
{
    T value;
    std::memcpy(&value, cursor, sizeof(T));
    cursor += sizeof(T);
    return qFromLittleEndian(value);
}

} // namespace

int Skeleton::indexOf(const QString &boneName) const
{
    for (int i = 0; i < bones.size(); ++i) {
        if (bones[i].name.compare(boneName, Qt::CaseInsensitive) == 0)
            return i;
    }
    return -1;
}

std::optional<Skeleton> Skeleton::fromJson(const QByteArray &json)
{
    const QJsonObject root = QJsonDocument::fromJson(json).object();
    if (root.isEmpty())
        return std::nullopt;

    Skeleton skeleton;
    skeleton.id = quint32(root.value("skeletonId").toInteger());
    for (const QJsonValue &bone : root.value("bones").toArray()) {
        const QJsonObject b = bone.toObject();
        skeleton.bones.append({b.value("name").toString(), b.value("parent").toInt(-1)});
    }
    for (const QJsonValue &m : root.value("bindPose").toArray()) {
        if (auto matrix = matrixFromJson(m))
            skeleton.bindPose.append(*matrix);
    }
    for (const QJsonValue &m : root.value("boneOffset").toArray())
        skeleton.boneOffset.append(matrixFromJson(m));
    return skeleton;
}

std::optional<Frame> Frame::parse(const QByteArray &payload)
{
    constexpr qsizetype headerSize = 8 + 8 + 4 * 5 + 4 + 2 + 2;
    if (payload.size() < headerSize)
        return std::nullopt;

    const char *cursor = payload.constData();
    Frame frame;
    frame.seq = readLE<quint64>(cursor);
    frame.monoNanos = readLE<qint64>(cursor);
    frame.animDelta = readLE<float>(cursor);
    const float x = readLE<float>(cursor);
    const float y = readLE<float>(cursor);
    const float z = readLE<float>(cursor);
    frame.position = {x, y, z};
    frame.renderedAngle = readLE<float>(cursor);
    frame.skeletonId = readLE<quint32>(cursor);
    frame.recording = (readLE<quint16>(cursor) & 1) != 0;
    const quint16 boneCount = readLE<quint16>(cursor);

    if (payload.size() != headerSize + qsizetype(boneCount) * 64)
        return std::nullopt;
    frame.bones.resize(boneCount);
    for (Matrix &m : frame.bones) {
        for (float &v : m)
            v = readLE<float>(cursor);
    }
    return frame;
}

std::optional<Hello> Hello::fromJson(const QByteArray &json)
{
    const QJsonObject root = QJsonDocument::fromJson(json).object();
    if (root.isEmpty())
        return std::nullopt;
    Hello hello;
    hello.protocol = root.value("protocol").toInt();
    hello.agentVersion = root.value("agent").toString();
    hello.gameVersion = root.value("game").toString();
    hello.revision = root.value("revision").toString();
    hello.supported = root.value("supported").toBool();
    return hello;
}

QByteArray encodeMessage(MsgType type, const QByteArray &payload)
{
    QByteArray message(4, Qt::Uninitialized);
    qToLittleEndian<quint32>(quint32(payload.size() + 1), message.data());
    message.append(char(type));
    message.append(payload);
    return message;
}

} // namespace pz

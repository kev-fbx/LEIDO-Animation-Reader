#include "export/TakeFile.h"

#include <QFile>
#include <QtEndian>

namespace {
constexpr char kMagic[8] = {'P', 'Z', 'R', 'E', 'C', 0, 1, 0};
}

double Take::durationSeconds() const
{
    return frames.size() < 2 ? 0.0 : double(frames.last().monoNanos - frames.first().monoNanos) / 1e9;
}

std::optional<TakeSummary> summarizeTake(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return std::nullopt;
    const QByteArray data = file.readAll();
    if (data.size() < 8 || !data.startsWith(QByteArray(kMagic, 8)))
        return std::nullopt;

    TakeSummary summary;
    qint64 firstNanos = 0, lastNanos = 0;
    qsizetype offset = 8;
    while (data.size() - offset >= 4) {
        const quint32 length = qFromLittleEndian<quint32>(data.constData() + offset);
        if (length < 1 || data.size() - offset - 4 < qsizetype(length))
            break;
        const auto type = pz::MsgType(quint8(data.at(offset + 4)));
        // FRAME payload: u64 seq, then i64 monoNanos.
        if (type == pz::MsgType::Frame && length >= 1 + 16) {
            const qint64 nanos = qFromLittleEndian<qint64>(data.constData() + offset + 5 + 8);
            if (summary.frames++ == 0)
                firstNanos = nanos;
            lastNanos = nanos;
        } else if (type == pz::MsgType::Hello && summary.gameVersion.isEmpty()) {
            if (auto hello = pz::Hello::fromJson(data.mid(offset + 5, length - 1)))
                summary.gameVersion = hello->gameVersion;
        }
        offset += 4 + length;
    }
    summary.seconds = double(lastNanos - firstNanos) / 1e9;
    return summary;
}

std::optional<Take> readTake(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = QStringLiteral("Cannot open %1: %2").arg(path, file.errorString());
        return std::nullopt;
    }
    const QByteArray data = file.readAll();
    if (data.size() < 8 || !data.startsWith(QByteArray(kMagic, 8))) {
        *error = QStringLiteral("%1 is not a .pzrec v1 file").arg(path);
        return std::nullopt;
    }

    Take take;
    take.sourcePath = path;
    QList<pz::Skeleton> skeletons;
    QList<pz::Frame> frames;

    qsizetype offset = 8;
    while (data.size() - offset >= 4) {
        const quint32 length = qFromLittleEndian<quint32>(data.constData() + offset);
        if (length < 1 || data.size() - offset - 4 < qsizetype(length))
            break; // truncated tail (e.g. the game was killed mid-write)
        const auto type = pz::MsgType(quint8(data.at(offset + 4)));
        const QByteArray payload = data.mid(offset + 5, length - 1);
        offset += 4 + length;

        if (type == pz::MsgType::Hello) {
            if (auto hello = pz::Hello::fromJson(payload))
                take.hello = *hello;
        } else if (type == pz::MsgType::Skeleton) {
            if (auto skeleton = pz::Skeleton::fromJson(payload))
                skeletons.append(*skeleton);
        } else if (type == pz::MsgType::Frame) {
            if (auto frame = pz::Frame::parse(payload))
                frames.append(*frame);
        }
    }

    if (frames.isEmpty()) {
        *error = QStringLiteral("%1 contains no frames").arg(path);
        return std::nullopt;
    }
    // Use the skeleton of the first frame; a mid-take skeleton change (model reset) ends the take there.
    const quint32 skeletonId = frames.first().skeletonId;
    for (const pz::Skeleton &skeleton : skeletons) {
        if (skeleton.id == skeletonId)
            take.skeleton = skeleton;
    }
    if (take.skeleton.bones.isEmpty()) {
        *error = QStringLiteral("%1 has no skeleton for its frames").arg(path);
        return std::nullopt;
    }
    for (const pz::Frame &frame : frames) {
        if (frame.skeletonId != skeletonId || frame.bones.size() != take.skeleton.bones.size())
            break;
        take.frames.append(frame);
    }
    return take;
}

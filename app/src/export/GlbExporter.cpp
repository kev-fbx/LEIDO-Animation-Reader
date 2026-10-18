#include "export/GlbExporter.h"

#include "scene/PzSpace.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QtEndian>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

// Spaces (see docs/protocol.md and the feasibility study):
//
//  * PZ model space: Y up, the character faces -Z with its left side at +X (a mirrored, left-handed
//    arrangement). Bone matrices are column-vector, translation in m03/m13/m23, row-major fields.
//  * Export space (glTF): right-handed, Y up, metres. Every model-space matrix is conjugated with
//    C = k * diag(1, 1, -1): mirroring Z un-mirrors the character and makes it face +Z.
//  * World: PZ places the model with Model.vectorToWorldCoords (mirror X, rotate by the rendered
//    angle, swap Y/Z, scale 1.5 / 0.61237). Working that through C gives the root node:
//    translation k * (x / 1.5, z / 0.61237, y / 1.5), rotation about +Y by -(renderedAngle + pi).

namespace {

constexpr int kGlbJson = 0x4E4F534A;
constexpr int kGlbBin = 0x004E4942;
constexpr int kFloat = 5126;
constexpr int kUnsignedShort = 5123;
constexpr int kUnsignedInt = 5125;
constexpr int kArrayBuffer = 34962;
constexpr int kElementArrayBuffer = 34963;

struct Trs {
    QVector3D t;
    QQuaternion r;
    QVector3D s{1, 1, 1};
};

QMatrix4x4 toMatrix(const pz::Matrix &m)
{
    return QMatrix4x4(m.data()); // QMatrix4x4 takes row-major values; PZ fields are row-major m00..m33
}

Trs decompose(const QMatrix4x4 &m)
{
    Trs trs;
    trs.t = m.column(3).toVector3D();
    QVector3D axes[3];
    for (int c = 0; c < 3; ++c) {
        axes[c] = m.column(c).toVector3D();
        trs.s[c] = axes[c].length();
    }
    float rot[9];
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c)
            rot[r * 3 + c] = trs.s[c] > 0 ? axes[c][r] / trs.s[c] : (r == c ? 1.f : 0.f);
    }
    trs.r = QQuaternion::fromRotationMatrix(QMatrix3x3(rot)).normalized();
    return trs;
}

// Appends typed data to the BIN chunk and describes it with a bufferView + accessor.
class BinBuilder
{
public:
    QByteArray bin;
    QJsonArray bufferViews;
    QJsonArray accessors;

    int addFloats(const QList<float> &values, int components, const char *type, bool withMinMax, int target = 0)
    {
        const int view = addView(reinterpret_cast<const char *>(values.constData()), values.size() * 4, target);
        QJsonObject accessor{{"bufferView", view}, {"componentType", kFloat},
                             {"count", int(values.size() / components)}, {"type", type}};
        if (withMinMax)
            addMinMax(accessor, values, components);
        accessors.append(accessor);
        return int(accessors.size() - 1);
    }

    int addUShorts(const QList<quint16> &values, const char *type, int components, int target = 0)
    {
        const int view = addView(reinterpret_cast<const char *>(values.constData()), values.size() * 2, target);
        accessors.append(QJsonObject{{"bufferView", view}, {"componentType", kUnsignedShort},
                                     {"count", int(values.size() / components)}, {"type", type}});
        return int(accessors.size() - 1);
    }

    int addIndices(const QList<quint32> &values)
    {
        const int view = addView(reinterpret_cast<const char *>(values.constData()), values.size() * 4,
                                 kElementArrayBuffer);
        accessors.append(QJsonObject{{"bufferView", view}, {"componentType", kUnsignedInt},
                                     {"count", int(values.size())}, {"type", "SCALAR"}});
        return int(accessors.size() - 1);
    }

    int addView(const char *data, qsizetype size, int target = 0)
    {
        while (bin.size() % 4)
            bin.append('\0');
        QJsonObject view{{"buffer", 0}, {"byteOffset", int(bin.size())}, {"byteLength", int(size)}};
        if (target)
            view.insert("target", target);
        bin.append(data, size);
        bufferViews.append(view);
        return int(bufferViews.size() - 1);
    }

private:
    static void addMinMax(QJsonObject &accessor, const QList<float> &values, int components)
    {
        QJsonArray mins, maxs;
        for (int c = 0; c < components; ++c) {
            float lo = std::numeric_limits<float>::max(), hi = std::numeric_limits<float>::lowest();
            for (qsizetype i = c; i < values.size(); i += components) {
                lo = std::min(lo, values[i]);
                hi = std::max(hi, values[i]);
            }
            mins.append(lo);
            maxs.append(hi);
        }
        accessor.insert("min", mins);
        accessor.insert("max", maxs);
    }
};

} // namespace

ExportResult exportGlb(const Take &take, const BodyMesh *body, const ExportOptions &options, const QString &outPath)
{
    using namespace pz::space;
    ExportResult result;
    const auto &bones = take.skeleton.bones;
    const int boneCount = int(bones.size());
    const int frameCount = int(take.frames.size());
    const float k = options.metresPerModelUnit;

    QMatrix4x4 C;
    C.scale(k, k, -k);
    const QMatrix4x4 Cinv = C.inverted();
    auto toExport = [&](const QMatrix4x4 &m) { return C * m * Cinv; };

    // --- Per-frame local joint transforms (export space) and root motion -------------------------
    QList<QList<Trs>> locals(frameCount, QList<Trs>(boneCount));
    QList<Trs> roots(frameCount);
    QList<double> times(frameCount);
    const auto &first = take.frames.first();
    const QVector3D originU(first.position.x() / kModelToTiles, first.position.z() / kModelToLevels,
                            first.position.y() / kModelToTiles);

    for (int f = 0; f < frameCount; ++f) {
        const pz::Frame &frame = take.frames[f];
        times[f] = double(frame.monoNanos - first.monoNanos) / 1e9;
        QList<QMatrix4x4> model(boneCount);
        for (int b = 0; b < boneCount; ++b)
            model[b] = toExport(toMatrix(frame.bones[b]));
        for (int b = 0; b < boneCount; ++b) {
            const int parent = bones[b].parent;
            locals[f][b] = decompose(parent < 0 ? model[b] : model[parent].inverted() * model[b]);
        }

        const QVector3D u(frame.position.x() / kModelToTiles, frame.position.z() / kModelToLevels,
                          frame.position.y() / kModelToTiles);
        QVector3D t = (u - originU) * k;
        if (!options.worldSpaceRoot) {
            t.setX(0);
            t.setZ(0);
        }
        const float angle = -(frame.renderedAngle + std::numbers::pi_v<float>);
        roots[f].t = t;
        roots[f].r = QQuaternion::fromAxisAndAngle(0, 1, 0, qRadiansToDegrees(angle));
    }

    // Self-check: our root * joint must land where PZ's own formula puts the bone (frame 0 and middle).
    if (options.worldSpaceRoot) {
        for (int f : {0, frameCount / 2}) {
            const pz::Frame &frame = take.frames[f];
            QMatrix4x4 root;
            root.translate(roots[f].t);
            root.rotate(roots[f].r);
            for (int b = 0; b < boneCount; ++b) {
                const QVector3D ours = (root * toExport(toMatrix(frame.bones[b]))).column(3).toVector3D();
                const QVector3D w = modelToWorld(pz::translationOf(frame.bones[b]), frame.renderedAngle, frame.position);
                const QVector3D theirs = (QVector3D(w.x() / kModelToTiles, w.z() / kModelToLevels, w.y() / kModelToTiles)
                                          - originU) * k;
                result.selfCheckErrorMetres = std::max(result.selfCheckErrorMetres, double((ours - theirs).length()));
            }
        }
    }

    // --- Resample to a fixed rate ----------------------------------------------------------------
    const double duration = times.last();
    const int samples = std::max(1, int(std::floor(duration * options.fps)) + 1);
    QList<float> sampleTimes(samples);
    QList<QList<Trs>> sampledLocals(samples, QList<Trs>(boneCount));
    QList<Trs> sampledRoots(samples);
    int cursor = 0;
    for (int s = 0; s < samples; ++s) {
        const double t = s / options.fps;
        sampleTimes[s] = float(t);
        while (cursor + 1 < frameCount - 1 && times[cursor + 1] <= t)
            ++cursor;
        const int next = std::min(cursor + 1, frameCount - 1);
        const double span = times[next] - times[cursor];
        const float a = span > 0 ? float(std::clamp((t - times[cursor]) / span, 0.0, 1.0)) : 0.f;
        auto blend = [a](const Trs &x, const Trs &y) {
            Trs out;
            out.t = x.t + (y.t - x.t) * a;
            out.s = x.s + (y.s - x.s) * a;
            out.r = QQuaternion::slerp(x.r, y.r, a);
            return out;
        };
        for (int b = 0; b < boneCount; ++b)
            sampledLocals[s][b] = blend(locals[cursor][b], locals[next][b]);
        sampledRoots[s] = blend(roots[cursor], roots[next]);
    }
    // Keep quaternion signs continuous so importers do not take the long way round.
    auto fixSigns = [](QList<Trs *> track) {
        for (int i = 1; i < track.size(); ++i) {
            if (QQuaternion::dotProduct(track[i - 1]->r, track[i]->r) < 0)
                track[i]->r = -track[i]->r;
        }
    };
    for (int b = 0; b < boneCount; ++b) {
        QList<Trs *> track;
        for (auto &frame : sampledLocals)
            track.append(&frame[b]);
        fixSigns(track);
    }
    {
        QList<Trs *> track;
        for (auto &r : sampledRoots)
            track.append(&r);
        fixSigns(track);
    }

    // --- Bind pose: from the game's bone offsets (inverse bind matrices, model space) ------------
    QList<QMatrix4x4> inverseBind(boneCount), bindModel(boneCount);
    for (int b = 0; b < boneCount; ++b) {
        const auto &offset = b < take.skeleton.boneOffset.size() ? take.skeleton.boneOffset[b] : std::nullopt;
        inverseBind[b] = offset ? toExport(toMatrix(*offset)) : QMatrix4x4();
        bindModel[b] = inverseBind[b].inverted();
    }

    // --- glTF JSON + BIN --------------------------------------------------------------------------
    BinBuilder bin;
    QJsonArray nodes;
    const int jointBase = 1;
    const int meshNodeIndex = jointBase + boneCount;

    QJsonArray rootChildren;
    for (int b = 0; b < boneCount; ++b) {
        if (bones[b].parent < 0)
            rootChildren.append(jointBase + b);
    }
    if (body)
        rootChildren.append(meshNodeIndex);
    nodes.append(QJsonObject{{"name", "PZ_Root"}, {"children", rootChildren}});

    auto trsJson = [](QJsonObject &node, const Trs &trs) {
        node.insert("translation", QJsonArray{trs.t.x(), trs.t.y(), trs.t.z()});
        node.insert("rotation", QJsonArray{trs.r.x(), trs.r.y(), trs.r.z(), trs.r.scalar()});
        node.insert("scale", QJsonArray{trs.s.x(), trs.s.y(), trs.s.z()});
    };
    for (int b = 0; b < boneCount; ++b) {
        QJsonObject node{{"name", bones[b].name}};
        QJsonArray children;
        for (int c = 0; c < boneCount; ++c) {
            if (bones[c].parent == b)
                children.append(jointBase + c);
        }
        if (!children.isEmpty())
            node.insert("children", children);
        const int parent = bones[b].parent;
        trsJson(node, decompose(parent < 0 ? bindModel[b] : bindModel[parent].inverted() * bindModel[b]));
        nodes.append(node);
    }

    QJsonObject gltf{{"asset", QJsonObject{{"version", "2.0"},
                                            {"generator", "LEIDO PZ Animation Exporter 0.1.0"},
                                            {"extras", QJsonObject{{"game", take.hello.gameVersion},
                                                                   {"revision", take.hello.revision},
                                                                   {"source", QFileInfo(take.sourcePath).fileName()},
                                                                   {"metresPerModelUnit", k}}}}},
                     {"scene", 0},
                     {"scenes", QJsonArray{QJsonObject{{"name", "PZ Take"}, {"nodes", QJsonArray{0}}}}}};

    // Skin
    QList<float> ibmData;
    for (const QMatrix4x4 &m : inverseBind) {
        for (int i = 0; i < 16; ++i)
            ibmData.append(m.constData()[i]); // QMatrix4x4 stores column-major, as glTF expects
    }
    QJsonArray jointList;
    for (int b = 0; b < boneCount; ++b)
        jointList.append(jointBase + b);
    QJsonObject skin{{"joints", jointList}, {"inverseBindMatrices", bin.addFloats(ibmData, 16, "MAT4", false)},
                     {"name", "PZ_Skeleton"}};
    for (int b = 0; b < boneCount; ++b) {
        if (bones[b].parent < 0) {
            skin.insert("skeleton", jointBase + b);
            break;
        }
    }

    // Mesh
    if (body) {
        QList<float> positions, normals, uvs, weights;
        QList<quint16> joints;
        QList<int> meshToSkeleton;
        for (const QString &name : body->boneNames)
            meshToSkeleton.append(std::max(0, take.skeleton.indexOf(name)));
        const QVector3D mirror(k, k, -k);
        for (const BodyMesh::Vertex &v : body->vertices) {
            const QVector3D p = v.position * mirror;
            const QVector3D n = QVector3D(v.normal.x(), v.normal.y(), -v.normal.z()).normalized();
            positions << p.x() << p.y() << p.z();
            normals << n.x() << n.y() << n.z();
            uvs << v.uv.x() << v.uv.y();
            for (int i = 0; i < 4; ++i) {
                joints << quint16(meshToSkeleton.value(v.bones[i], 0));
                weights << v.weights[i];
            }
        }
        QList<quint32> indices;
        for (qsizetype i = 0; i + 2 < body->indices.size(); i += 3)
            indices << body->indices[i] << body->indices[i + 2] << body->indices[i + 1]; // mirror flips winding

        // glTF front faces are counter-clockwise. Check each triangle's geometric normal against its
        // vertex normals; if most disagree, the winding is backwards, so flip it.
        auto vec = [&](quint32 i) { return QVector3D(positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]); };
        auto nrm = [&](quint32 i) { return QVector3D(normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]); };
        int agree = 0, triangles = 0;
        for (qsizetype i = 0; i + 2 < indices.size(); i += 3) {
            const QVector3D face = QVector3D::crossProduct(vec(indices[i + 1]) - vec(indices[i]),
                                                           vec(indices[i + 2]) - vec(indices[i]));
            if (face.lengthSquared() <= 0)
                continue;
            ++triangles;
            if (QVector3D::dotProduct(face, nrm(indices[i]) + nrm(indices[i + 1]) + nrm(indices[i + 2])) > 0)
                ++agree;
        }
        result.windingAgreement = triangles ? double(agree) / triangles : 1.0;
        if (result.windingAgreement < 0.5) {
            for (qsizetype i = 0; i + 2 < indices.size(); i += 3)
                std::swap(indices[i + 1], indices[i + 2]);
            result.windingFlipped = true;
        }

        QJsonObject attributes{{"POSITION", bin.addFloats(positions, 3, "VEC3", true, kArrayBuffer)},
                               {"NORMAL", bin.addFloats(normals, 3, "VEC3", false, kArrayBuffer)},
                               {"TEXCOORD_0", bin.addFloats(uvs, 2, "VEC2", false, kArrayBuffer)},
                               {"JOINTS_0", bin.addUShorts(joints, "VEC4", 4, kArrayBuffer)},
                               {"WEIGHTS_0", bin.addFloats(weights, 4, "VEC4", false, kArrayBuffer)}};
        QJsonObject primitive{{"attributes", attributes}, {"indices", bin.addIndices(indices)}};

        QJsonObject pbr{{"metallicFactor", 0.0}, {"roughnessFactor", 1.0}};
        QFile texture(body->texturePath);
        if (!body->texturePath.isEmpty() && texture.open(QIODevice::ReadOnly)) {
            const QByteArray png = texture.readAll();
            const int view = bin.addView(png.constData(), png.size());
            gltf.insert("images", QJsonArray{QJsonObject{{"bufferView", view}, {"mimeType", "image/png"},
                                                         {"name", QFileInfo(body->texturePath).baseName()}}});
            gltf.insert("samplers", QJsonArray{QJsonObject{{"magFilter", 9729}, {"minFilter", 9987}}});
            gltf.insert("textures", QJsonArray{QJsonObject{{"source", 0}, {"sampler", 0}}});
            pbr.insert("baseColorTexture", QJsonObject{{"index", 0}});
        }
        // PZ draws bodies with alpha testing: transparent texels hide geometry such as the dress panel
        // (CharacterMask part "Dress"), which only clothing masks turn on. MASK reproduces that.
        gltf.insert("materials", QJsonArray{QJsonObject{{"name", "PZ_Body"}, {"pbrMetallicRoughness", pbr},
                                                        {"alphaMode", "MASK"}, {"alphaCutoff", 0.5}}});
        primitive.insert("material", 0);
        gltf.insert("meshes", QJsonArray{QJsonObject{{"name", "PZ_Body"}, {"primitives", QJsonArray{primitive}}}});
        nodes.append(QJsonObject{{"name", "PZ_Body"}, {"mesh", 0}, {"skin", 0}});
    }

    // Animation
    QJsonArray samplers, channels;
    const int timeAccessor = bin.addFloats(sampleTimes, 1, "SCALAR", true);
    auto addChannel = [&](int node, const char *path, const QList<float> &values, int components, const char *type) {
        samplers.append(QJsonObject{{"input", timeAccessor},
                                    {"output", bin.addFloats(values, components, type, false)},
                                    {"interpolation", "LINEAR"}});
        channels.append(QJsonObject{{"sampler", int(samplers.size() - 1)},
                                    {"target", QJsonObject{{"node", node}, {"path", path}}}});
    };
    auto addTrsChannels = [&](int node, auto get, bool withScale) {
        QList<float> t, r, s;
        for (int i = 0; i < samples; ++i) {
            const Trs &trs = get(i);
            t << trs.t.x() << trs.t.y() << trs.t.z();
            r << trs.r.x() << trs.r.y() << trs.r.z() << trs.r.scalar();
            s << trs.s.x() << trs.s.y() << trs.s.z();
        }
        addChannel(node, "translation", t, 3, "VEC3");
        addChannel(node, "rotation", r, 4, "VEC4");
        if (withScale)
            addChannel(node, "scale", s, 3, "VEC3");
    };
    addTrsChannels(0, [&](int i) -> const Trs & { return sampledRoots[i]; }, false);
    for (int b = 0; b < boneCount; ++b)
        addTrsChannels(jointBase + b, [&](int i) -> const Trs & { return sampledLocals[i][b]; }, true);
    const QString takeName = QFileInfo(take.sourcePath).completeBaseName();
    gltf.insert("animations", QJsonArray{QJsonObject{{"name", takeName.isEmpty() ? "Take" : takeName},
                                                     {"samplers", samplers}, {"channels", channels}}});

    gltf.insert("nodes", nodes);
    gltf.insert("skins", QJsonArray{skin});
    gltf.insert("accessors", bin.accessors);
    gltf.insert("bufferViews", bin.bufferViews);
    while (bin.bin.size() % 4)
        bin.bin.append('\0');
    gltf.insert("buffers", QJsonArray{QJsonObject{{"byteLength", int(bin.bin.size())}}});

    // --- GLB container ------------------------------------------------------------------------------
    QByteArray json = QJsonDocument(gltf).toJson(QJsonDocument::Compact);
    while (json.size() % 4)
        json.append(' ');
    QByteArray glb;
    auto putU32 = [&glb](quint32 v) {
        char b[4];
        qToLittleEndian(v, b);
        glb.append(b, 4);
    };
    putU32(0x46546C67); // "glTF"
    putU32(2);
    putU32(quint32(12 + 8 + json.size() + 8 + bin.bin.size()));
    putU32(quint32(json.size()));
    putU32(kGlbJson);
    glb.append(json);
    putU32(quint32(bin.bin.size()));
    putU32(kGlbBin);
    glb.append(bin.bin);

    QFile out(outPath);
    if (!out.open(QIODevice::WriteOnly) || out.write(glb) != glb.size()) {
        result.message = QStringLiteral("Cannot write %1: %2").arg(outPath, out.errorString());
        return result;
    }
    result.ok = true;
    result.sampledFrames = samples;
    result.seconds = duration;
    result.message = QStringLiteral("Wrote %1 (%2 frames at %3 fps, %4 s, %5 KB)")
                         .arg(outPath)
                         .arg(samples)
                         .arg(options.fps)
                         .arg(duration, 0, 'f', 2)
                         .arg(glb.size() / 1024);
    return result;
}

#include "export/BodyMesh.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>

QByteArray stripEmptySkinWeights(const QByteArray &xFile, int *removed)
{
    static const QRegularExpression emptyBlock(
        QStringLiteral(R"(SkinWeights\s*\{\s*"[^"]*"\s*;\s*0\s*;\s*;\s*;\s*[^}]*\})"));
    QString text = QString::fromLatin1(xFile);
    *removed = int(text.count(emptyBlock));
    text.remove(emptyBlock);
    return text.toLatin1();
}

namespace {

QString resolveTexture(const QString &fileName, const QString &pzInstallDir)
{
    if (fileName.isEmpty())
        return {};
    const QString name = QFileInfo(fileName).fileName();
    const QDir textures(QDir(pzInstallDir).filePath("media/textures"));
    for (const QString &candidate : {textures.filePath("Body/" + name), textures.filePath(name)}) {
        if (QFileInfo::exists(candidate))
            return candidate;
    }
    QDirIterator it(textures.path(), {name}, QDir::Files, QDirIterator::Subdirectories);
    return it.hasNext() ? it.next() : QString();
}

} // namespace

std::optional<BodyMesh> loadBodyMesh(const QString &path, const QString &pzInstallDir, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = QStringLiteral("Cannot open %1: %2").arg(QDir::toNativeSeparators(path), file.errorString());
        return std::nullopt;
    }
    int removed = 0;
    const QByteArray data = stripEmptySkinWeights(file.readAll(), &removed);

    Assimp::Importer importer;
    // Assimp returns UVs with a bottom-left origin; FlipUVs restores the file's top-left origin,
    // which is also glTF's. (Without it the texture renders upside down: underwear on the chest.)
    const unsigned flags = aiProcess_Triangulate | aiProcess_LimitBoneWeights | aiProcess_GenSmoothNormals
                         | aiProcess_FlipUVs;
    const aiScene *scene = importer.ReadFileFromMemory(data.constData(), size_t(data.size()), flags, "x");
    if (!scene || !scene->mRootNode || scene->mNumMeshes == 0) {
        *error = QStringLiteral("Assimp could not read %1: %2")
                     .arg(QFileInfo(path).fileName(), QString::fromUtf8(importer.GetErrorString()).left(200));
        return std::nullopt;
    }

    BodyMesh mesh;
    mesh.sourcePath = path;
    for (unsigned m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh *src = scene->mMeshes[m];
        const quint32 base = quint32(mesh.vertices.size());

        for (unsigned v = 0; v < src->mNumVertices; ++v) {
            BodyMesh::Vertex vertex;
            // Mirror Z: Assimp space -> PZ model space.
            vertex.position = {src->mVertices[v].x, src->mVertices[v].y, -src->mVertices[v].z};
            if (src->HasNormals())
                vertex.normal = {src->mNormals[v].x, src->mNormals[v].y, -src->mNormals[v].z};
            if (src->HasTextureCoords(0))
                vertex.uv = {src->mTextureCoords[0][v].x, src->mTextureCoords[0][v].y};
            vertex.bones.fill(-1);
            mesh.vertices.append(vertex);
        }

        for (unsigned b = 0; b < src->mNumBones; ++b) {
            const aiBone *bone = src->mBones[b];
            const QString name = QString::fromUtf8(bone->mName.C_Str());
            qsizetype boneIndex = mesh.boneNames.indexOf(name);
            if (boneIndex < 0) {
                boneIndex = mesh.boneNames.size();
                mesh.boneNames.append(name);
                // S * O * S with S = diag(1, 1, -1): negate row 2 and column 2 except their shared element.
                const aiMatrix4x4 &o = bone->mOffsetMatrix;
                mesh.boneOffsets.append({o.a1, o.a2, -o.a3, o.a4, o.b1, o.b2, -o.b3, o.b4,
                                         -o.c1, -o.c2, o.c3, -o.c4, o.d1, o.d2, o.d3, o.d4});
            }
            for (unsigned w = 0; w < bone->mNumWeights; ++w) {
                BodyMesh::Vertex &vertex = mesh.vertices[base + bone->mWeights[w].mVertexId];
                // Keep the four largest influences (LimitBoneWeights already caps at four).
                auto slot = std::min_element(vertex.weights.begin(), vertex.weights.end());
                if (bone->mWeights[w].mWeight > *slot) {
                    const auto i = slot - vertex.weights.begin();
                    vertex.weights[i] = bone->mWeights[w].mWeight;
                    vertex.bones[i] = int(boneIndex);
                }
            }
        }

        for (unsigned f = 0; f < src->mNumFaces; ++f) {
            const aiFace &face = src->mFaces[f];
            if (face.mNumIndices == 3) {
                // The mirror flips handedness, so reverse the winding with it.
                mesh.indices << base + face.mIndices[0] << base + face.mIndices[2] << base + face.mIndices[1];
            }
        }

        if (mesh.texturePath.isEmpty() && src->mMaterialIndex < scene->mNumMaterials) {
            aiString texture;
            if (scene->mMaterials[src->mMaterialIndex]->GetTexture(aiTextureType_DIFFUSE, 0, &texture) == AI_SUCCESS)
                mesh.texturePath = resolveTexture(QString::fromUtf8(texture.C_Str()), pzInstallDir);
        }
    }

    // Normalise weights; unused slots point at bone 0 with weight 0 (valid glTF).
    for (BodyMesh::Vertex &vertex : mesh.vertices) {
        float sum = 0;
        for (float w : vertex.weights)
            sum += w;
        for (int i = 0; i < 4; ++i) {
            if (vertex.bones[i] < 0 || sum <= 0) {
                vertex.bones[i] = 0;
                vertex.weights[i] = 0;
            } else {
                vertex.weights[i] /= sum;
            }
        }
        if (sum <= 0)
            vertex.weights[0] = 1; // unweighted vertex: follow bone 0
    }
    return mesh;
}

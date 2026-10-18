#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QVector2D>
#include <QVector3D>

#include <array>
#include <optional>

// A PZ skinned mesh (.x) loaded with Assimp and converted into PZ model space, the space of the
// agent's bone matrices. Assimp's space is PZ's mirrored on Z (measured: game offset = S * assimp
// offset * S with S = diag(1, 1, -1)), so positions, normals and offsets are mirrored back here.
struct BodyMesh {
    struct Vertex {
        QVector3D position;
        QVector3D normal;
        QVector2D uv;
        std::array<int, 4> bones{};      // indices into boneNames
        std::array<float, 4> weights{};  // sums to 1
    };

    QString sourcePath;
    QList<Vertex> vertices;
    QList<quint32> indices; // triangles; winding as in the file after the mirror (exporter re-checks it)
    QStringList boneNames;  // bones that carry weights
    QList<std::array<float, 16>> boneOffsets; // inverse bind per boneNames entry, PZ model space, row-major
    QString texturePath;    // resolved diffuse texture, empty if not found
};

// Loads a PZ .x mesh. pzInstallDir is used to resolve its texture under media/textures.
std::optional<BodyMesh> loadBodyMesh(const QString &path, const QString &pzInstallDir, QString *error);

// PZ's exporter writes SkinWeights blocks with zero weights (e.g. "Bip01_BackPack") whose empty arrays
// Assimp's DirectX parser rejects. Those bones influence no vertex, so the blocks are dropped.
QByteArray stripEmptySkinWeights(const QByteArray &xFile, int *removed);

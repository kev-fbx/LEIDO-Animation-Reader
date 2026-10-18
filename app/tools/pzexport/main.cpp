// pzexport: converts a .pzrec take into a GLB without the GUI. Used for testing and batch work.
//
//   pzexport <take.pzrec> [out.glb] [--fps 30] [--scale 2.0] [--in-place] [--no-mesh] [--pz <install dir>]

#include "export/BodyMesh.h"
#include "export/GlbExporter.h"
#include "export/TakeFile.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMatrix4x4>
#include <QTextStream>

#include <cmath>

namespace {

QTextStream &out()
{
    static QTextStream stream(stdout);
    return stream;
}

// Compares Assimp's offset matrices with the game's bone offsets: equal means the mesh vertices live
// in the same space as the streamed bone matrices, which skinning relies on.
void compareOffsets(const Take &take, const BodyMesh &mesh)
{
    double worst = 0;
    QString worstBone;
    int missing = 0;
    for (qsizetype i = 0; i < mesh.boneNames.size(); ++i) {
        const int b = take.skeleton.indexOf(mesh.boneNames[i]);
        if (b < 0 || b >= take.skeleton.boneOffset.size() || !take.skeleton.boneOffset[b]) {
            ++missing;
            continue;
        }
        const auto &game = *take.skeleton.boneOffset[b];
        for (int e = 0; e < 16; ++e) {
            const double d = std::abs(double(game[e]) - double(mesh.boneOffsets[i][e]));
            if (d > worst) {
                worst = d;
                worstBone = mesh.boneNames[i];
            }
        }
    }
    if (qEnvironmentVariableIsSet("PZEXPORT_DUMP_OFFSETS")) {
        for (const QString name : {"Bip01", "Bip01_Pelvis", "Bip01_Head", "Bip01_R_Hand"}) {
            const qsizetype i = mesh.boneNames.indexOf(name);
            const int b = take.skeleton.indexOf(name);
            if (i < 0 || b < 0 || !take.skeleton.boneOffset[b])
                continue;
            out() << name << "\n  assimp:";
            for (float v : mesh.boneOffsets[i])
                out() << " " << QString::number(v, 'f', 4);
            out() << "\n  game:  ";
            for (float v : *take.skeleton.boneOffset[b])
                out() << " " << QString::number(v, 'f', 4);
            out() << "\n";
        }
    }
    out() << "Mesh vs game bone offsets: max element difference " << worst << " (" << worstBone << ")";
    if (missing)
        out() << ", " << missing << " mesh bone(s) not in the game skeleton";
    out() << (worst < 1e-3 ? "  -> same space\n" : "  -> SPACES DIFFER\n");
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments().mid(1);

    ExportOptions options;
    QString pzDir = "C:/Program Files (x86)/Steam/steamapps/common/ProjectZomboid";
    bool withMesh = true;
    QStringList positional;
    for (qsizetype i = 0; i < args.size(); ++i) {
        const QString &a = args[i];
        if (a == "--fps" && i + 1 < args.size())
            options.fps = args[++i].toDouble();
        else if (a == "--scale" && i + 1 < args.size())
            options.metresPerModelUnit = args[++i].toFloat();
        else if (a == "--in-place")
            options.worldSpaceRoot = false;
        else if (a == "--no-mesh")
            withMesh = false;
        else if (a == "--pz" && i + 1 < args.size())
            pzDir = args[++i];
        else
            positional << a;
    }
    if (positional.isEmpty() || options.fps <= 0 || options.metresPerModelUnit <= 0) {
        out() << "usage: pzexport <take.pzrec> [out.glb] [--fps 30] [--scale 2.0] [--in-place] [--no-mesh] [--pz dir]\n";
        return 2;
    }
    const QString takePath = positional[0];
    const QString outPath = positional.value(1, QFileInfo(takePath).path() + "/" + QFileInfo(takePath).completeBaseName() + ".glb");

    QString error;
    const auto take = readTake(takePath, &error);
    if (!take) {
        out() << error << "\n";
        return 1;
    }
    out() << "Take: " << take->frames.size() << " frames, " << take->durationSeconds() << " s, "
          << take->skeleton.bones.size() << " bones, game " << take->hello.gameVersion << "\n";

    std::optional<BodyMesh> mesh;
    if (withMesh) {
        mesh = loadBodyMesh(QDir(pzDir).filePath("media/models_X/Skinned/MaleBody.x"), pzDir, &error);
        if (!mesh) {
            out() << error << "\n";
            return 1;
        }
        out() << "Mesh: " << mesh->vertices.size() << " vertices, " << mesh->indices.size() / 3 << " triangles, "
              << mesh->boneNames.size() << " bones, texture "
              << (mesh->texturePath.isEmpty() ? QStringLiteral("(not found)") : QDir::toNativeSeparators(mesh->texturePath))
              << "\n";
        compareOffsets(*take, *mesh);
    }

    const ExportResult result = exportGlb(*take, mesh ? &*mesh : nullptr, options, outPath);
    out() << result.message << "\n";
    if (result.ok && options.worldSpaceRoot)
        out() << "Self-check vs PZ world formula: max joint error " << result.selfCheckErrorMetres * 1000 << " mm\n";
    if (result.ok && mesh)
        out() << "Triangle winding vs normals: " << QString::number(result.windingAgreement * 100, 'f', 1)
              << "% agree" << (result.windingFlipped ? " -> flipped all triangles\n" : "\n");
    return result.ok ? 0 : 1;
}

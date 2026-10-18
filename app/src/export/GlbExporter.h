#pragma once

#include "export/BodyMesh.h"
#include "export/TakeFile.h"

#include <QString>

struct ExportOptions {
    double fps = 30.0;
    // Model units -> metres. 0.824 units from feet to head joint suggests ~2.0 (to be confirmed).
    float metresPerModelUnit = 2.0f;
    // true: the character travels through the scene as in the game; false: horizontal motion removed.
    bool worldSpaceRoot = true;
};

struct ExportResult {
    bool ok = false;
    QString message;
    int sampledFrames = 0;
    double seconds = 0;
    double selfCheckErrorMetres = 0; // max joint-position error against PZ's own world formula
    double windingAgreement = 1;     // share of triangles whose winding matched their vertex normals
    bool windingFlipped = false;     // true if the exporter reversed all triangles to fix that
};

// Writes a binary glTF: textured skinned body, one animation, root motion on "PZ_Root".
// body may be null (armature-only export).
ExportResult exportGlb(const Take &take, const BodyMesh *body, const ExportOptions &options, const QString &outPath);

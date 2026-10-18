#pragma once

// Coordinate conversions taken from the game (zombie.core.skinnedmodel.model.Model.vectorToWorldCoords,
// zombie.iso.Vector3.rotatey) so the viewport places bones exactly where PZ renders them.

#include <QVector3D>

#include <cmath>

namespace pz::space {

// Model units -> tiles (horizontal) and -> levels (vertical).
constexpr float kModelToTiles = 1.5f;
constexpr float kModelToLevels = 0.61237234f;
// Height of one level expressed in tiles, so the viewport is not squashed vertically.
constexpr float kLevelHeightInTiles = kModelToTiles / kModelToLevels;
// Viewport scene units per tile (Qt Quick 3D works comfortably at this scale).
constexpr float kSceneUnitsPerTile = 100.0f;

// Model.vectorToWorldCoords: model-space point -> PZ world (x, y in tiles; z in levels).
inline QVector3D modelToWorld(const QVector3D &model, float renderedAngle, const QVector3D &characterPos)
{
    const float x = -model.x();
    const float z = model.z();
    const float c = std::cos(renderedAngle);
    const float s = std::sin(renderedAngle);
    const float rx = x * c - z * s;
    const float rz = x * s + z * c;
    // After rotation PZ swaps y and z: world y = rotated z, world z = model y.
    return {rx * kModelToTiles + characterPos.x(),
            rz * kModelToTiles + characterPos.y(),
            model.y() * kModelToLevels + characterPos.z()};
}

// PZ world -> viewport scene (Y up). Tiles map to X and Z, levels to Y.
inline QVector3D worldToScene(const QVector3D &world)
{
    return QVector3D(world.x(), world.z() * kLevelHeightInTiles, world.y()) * kSceneUnitsPerTile;
}

} // namespace pz::space

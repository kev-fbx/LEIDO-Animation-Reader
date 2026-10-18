#pragma once

#include "protocol/Protocol.h"

#include <QList>
#include <QString>

#include <optional>

// A recorded take (.pzrec): HELLO, SKELETON and FRAME messages exactly as the agent sent them.
struct Take {
    pz::Hello hello;
    pz::Skeleton skeleton;   // the skeleton the frames below refer to
    QList<pz::Frame> frames; // only frames for `skeleton`, in recording order
    QString sourcePath;

    double durationSeconds() const;
};

// Returns std::nullopt and sets *error on failure.
std::optional<Take> readTake(const QString &path, QString *error);

// Cheap scan for listings: walks message headers only, never decodes bone matrices.
struct TakeSummary {
    int frames = 0;
    double seconds = 0;
    QString gameVersion;
};
std::optional<TakeSummary> summarizeTake(const QString &path);

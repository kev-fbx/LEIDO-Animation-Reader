#include "app/TakeLibrary.h"

#include "app/AppSettings.h"
#include "export/TakeFile.h"

#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QPointer>
#include <QThread>

TakeLibrary::TakeLibrary(AppSettings *settings, QObject *parent)
    : QAbstractListModel(parent)
    , m_settings(settings)
{
    // Coalesce bursts of file-system events (a take being written, a GLB being exported).
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(400);
    connect(&m_debounce, &QTimer::timeout, this, &TakeLibrary::refresh);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, &m_debounce, qOverload<>(&QTimer::start));
    connect(m_settings, &AppSettings::outputDirChanged, this, &TakeLibrary::refresh);
    refresh();
}

int TakeLibrary::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_entries.size());
}

QVariant TakeLibrary::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};
    const Entry &e = m_entries[index.row()];
    switch (role) {
    case NameRole:
        return e.name;
    case PathRole:
        return QDir::toNativeSeparators(e.path);
    case ModifiedRole:
        return QLocale().toString(e.modified, QLocale::ShortFormat);
    case SizeTextRole:
        return QLocale().formattedDataSize(e.size, 1);
    case DurationTextRole:
        if (e.frames < 0)
            return QStringLiteral("...");
        return QStringLiteral("%1:%2")
            .arg(int(e.seconds) / 60)
            .arg(int(e.seconds) % 60, 2, 10, QLatin1Char('0'));
    case FramesRole:
        return e.frames;
    case GlbPathRole:
        return QDir::toNativeSeparators(glbPathFor(e));
    case HasGlbRole:
        return QFileInfo::exists(glbPathFor(e));
    case CheckedRole:
        return e.checked;
    default:
        return {};
    }
}

bool TakeLibrary::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != CheckedRole || !index.isValid() || index.row() >= m_entries.size())
        return false;
    m_entries[index.row()].checked = value.toBool();
    emit dataChanged(index, index, {CheckedRole});
    emit checkedChanged();
    return true;
}

QHash<int, QByteArray> TakeLibrary::roleNames() const
{
    return {{NameRole, "name"},         {PathRole, "path"},       {ModifiedRole, "modified"},
            {SizeTextRole, "sizeText"}, {DurationTextRole, "duration"}, {FramesRole, "frames"},
            {GlbPathRole, "glbPath"},   {HasGlbRole, "hasGlb"},   {CheckedRole, "checked"}};
}

int TakeLibrary::checkedCount() const
{
    return int(std::count_if(m_entries.cbegin(), m_entries.cend(), [](const Entry &e) { return e.checked; }));
}

QStringList TakeLibrary::checkedPaths() const
{
    QStringList paths;
    for (const Entry &e : m_entries) {
        if (e.checked)
            paths << e.path;
    }
    return paths;
}

QStringList TakeLibrary::pathsWithoutGlb() const
{
    QStringList paths;
    for (const Entry &e : m_entries) {
        if (!QFileInfo::exists(glbPathFor(e)))
            paths << e.path;
    }
    return paths;
}

void TakeLibrary::setChecked(int row, bool checked)
{
    setData(index(row), checked, CheckedRole);
}

void TakeLibrary::setAllChecked(bool checked)
{
    for (Entry &e : m_entries)
        e.checked = checked;
    if (!m_entries.isEmpty())
        emit dataChanged(index(0), index(int(m_entries.size()) - 1), {CheckedRole});
    emit checkedChanged();
}

void TakeLibrary::refresh()
{
    const QString rawDir = m_settings->rawTakesDir();
    QDir().mkpath(rawDir);
    // Watch both folders: takes appear in raw/, GLBs in the output folder.
    if (!m_watcher.directories().isEmpty())
        m_watcher.removePaths(m_watcher.directories());
    m_watcher.addPaths({rawDir, m_settings->outputDir()});

    // Keep check marks and already-scanned details for files that did not change.
    QHash<QString, Entry> previous;
    for (const Entry &e : std::as_const(m_entries))
        previous.insert(e.path, e);

    QList<Entry> entries;
    const auto files = QDir(rawDir).entryInfoList({"*.pzrec"}, QDir::Files, QDir::Time);
    for (const QFileInfo &info : files) {
        Entry e;
        e.path = info.absoluteFilePath();
        e.name = info.completeBaseName();
        e.modified = info.lastModified();
        e.size = info.size();
        const auto old = previous.constFind(e.path);
        if (old != previous.cend()) {
            e.checked = old->checked;
            if (old->size == e.size && old->modified == e.modified) {
                e.frames = old->frames;
                e.seconds = old->seconds;
            }
        }
        entries.append(e);
    }

    const bool sameFiles = entries.size() == m_entries.size()
        && std::equal(entries.cbegin(), entries.cend(), m_entries.cbegin(),
                      [](const Entry &a, const Entry &b) { return a.path == b.path; });
    if (sameFiles) {
        // Same files (e.g. a take still being written): update rows in place, no reset or flicker.
        m_entries = entries;
        if (!m_entries.isEmpty())
            emit dataChanged(index(0), index(int(m_entries.size()) - 1));
    } else {
        beginResetModel();
        m_entries = entries;
        endResetModel();
        emit countChanged();
        emit checkedChanged();
    }
    scanDetails(entries);
}

void TakeLibrary::scanDetails(QList<Entry> entries)
{
    entries.removeIf([](const Entry &e) { return e.frames >= 0; });
    if (entries.isEmpty())
        return;
    const quint64 generation = ++m_generation;
    QPointer<TakeLibrary> self(this);
    QThread *worker = QThread::create([self, entries, generation] {
        for (const Entry &e : entries) {
            const auto summary = summarizeTake(e.path);
            const int frames = summary ? summary->frames : 0;
            const double seconds = summary ? summary->seconds : 0;
            QMetaObject::invokeMethod(self, [self, path = e.path, frames, seconds, generation] {
                if (!self || generation != self->m_generation)
                    return; // a newer refresh replaced the list
                for (int row = 0; row < self->m_entries.size(); ++row) {
                    if (self->m_entries[row].path == path) {
                        self->m_entries[row].frames = frames;
                        self->m_entries[row].seconds = seconds;
                        emit self->dataChanged(self->index(row), self->index(row), {FramesRole, DurationTextRole});
                    }
                }
            });
        }
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

QString TakeLibrary::glbPathFor(const Entry &entry) const
{
    return QDir(m_settings->outputDir()).filePath(entry.name + ".glb");
}

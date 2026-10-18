#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class AppSettings;

// The .pzrec takes in <output>/raw, with duration and whether a GLB already exists. Refreshes itself
// when the folder changes (e.g. a new recording) or the output folder setting changes.
class TakeLibrary : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created in main.cpp")

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int checkedCount READ checkedCount NOTIFY checkedChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        PathRole,
        ModifiedRole,
        SizeTextRole,
        DurationTextRole,
        FramesRole,
        GlbPathRole,
        HasGlbRole,
        CheckedRole,
    };

    explicit TakeLibrary(AppSettings *settings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

    int checkedCount() const;
    Q_INVOKABLE QStringList checkedPaths() const;
    Q_INVOKABLE QStringList pathsWithoutGlb() const;
    Q_INVOKABLE void setAllChecked(bool checked);
    Q_INVOKABLE void setChecked(int row, bool checked);
    Q_INVOKABLE void refresh();

signals:
    void countChanged();
    void checkedChanged();

private:
    struct Entry {
        QString path;
        QString name;
        QDateTime modified;
        qint64 size = 0;
        int frames = -1; // -1: not scanned yet
        double seconds = 0;
        bool checked = false;
    };

    void scanDetails(QList<Entry> entries);
    QString glbPathFor(const Entry &entry) const;

    AppSettings *m_settings;
    QList<Entry> m_entries;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    quint64 m_generation = 0;
};

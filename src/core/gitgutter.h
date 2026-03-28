#ifndef GITGUTTER_H
#define GITGUTTER_H

#include <QObject>
#include <QVector>
#include <QProcess>
#include <QString>

struct GitLineChange {
    enum Type { Added, Modified, Deleted };
    Type type;
    int startLine;
    int lineCount;
};

class GitGutter : public QObject
{
    Q_OBJECT

public:
    explicit GitGutter(QObject *parent = nullptr);

    /// Re-compute gutter marks for the given file.
    /// Uses `git diff -U0 HEAD` comparing working-tree content.
    void updateForFile(const QString &filePath, const QString &currentContent);

    QVector<GitLineChange> changes() const { return m_changes; }

    /// Returns the change type for the given 1-based line number.
    /// If the line has no change, returns Added with lineCount 0
    /// (caller should check hasChangeAt() first).
    GitLineChange::Type lineStatus(int line) const;

    /// Whether the given 1-based line has a recorded change.
    bool hasChangeAt(int line) const;

    bool hasChanges() const { return !m_changes.isEmpty(); }

signals:
    void gutterUpdated();

private:
    void parseDiff(const QString &diffOutput);
    QString repoRootFor(const QString &filePath) const;

    QVector<GitLineChange> m_changes;
    QString m_filePath;
};

#endif // GITGUTTER_H

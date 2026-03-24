#ifndef GITSTATUSDELEGATE_H
#define GITSTATUSDELEGATE_H

#include <QStyledItemDelegate>
#include <QMap>
#include <QString>

enum class GitStatus {
    Untracked,
    Modified,
    Added,
    Deleted,
    Renamed,
    Conflicted,
    Ignored,
    Clean
};

class GitStatusDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit GitStatusDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    // Set Git status for a file
    void setFileStatus(const QString &filePath, GitStatus status);

    // Clear all statuses
    void clearStatuses();

    // Update statuses from Git repository at path
    void updateFromRepository(const QString &repoPath);

private:
    QMap<QString, GitStatus> m_fileStatuses;

    QString getStatusSymbol(GitStatus status) const;
    QColor getStatusColor(GitStatus status) const;
};

#endif // GITSTATUSDELEGATE_H

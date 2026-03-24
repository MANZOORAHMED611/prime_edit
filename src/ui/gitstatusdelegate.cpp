#include "gitstatusdelegate.h"
#include <QPainter>
#include <QFileSystemModel>
#include <QProcess>
#include <QDir>
#include <QFileInfo>

GitStatusDelegate::GitStatusDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void GitStatusDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    // Draw the standard item
    QStyledItemDelegate::paint(painter, option, index);

    // Get file path
    const QFileSystemModel *fsModel = qobject_cast<const QFileSystemModel*>(index.model());
    if (!fsModel) return;

    QString filePath = fsModel->filePath(index);
    if (filePath.isEmpty()) return;

    // Check if we have status for this file
    if (!m_fileStatuses.contains(filePath)) return;

    GitStatus status = m_fileStatuses[filePath];
    if (status == GitStatus::Clean) return; // Don't show anything for clean files

    // Draw status indicator
    painter->save();

    QString symbol = getStatusSymbol(status);
    QColor color = getStatusColor(status);

    // Draw in the left margin
    QRect statusRect = option.rect;
    statusRect.setLeft(statusRect.left() + 2);
    statusRect.setWidth(16);

    painter->setPen(color);
    QFont font = painter->font();
    font.setBold(true);
    font.setPointSize(font.pointSize() - 1);
    painter->setFont(font);

    painter->drawText(statusRect, Qt::AlignLeft | Qt::AlignVCenter, symbol);

    painter->restore();
}

void GitStatusDelegate::setFileStatus(const QString &filePath, GitStatus status)
{
    m_fileStatuses[filePath] = status;
}

void GitStatusDelegate::clearStatuses()
{
    m_fileStatuses.clear();
}

void GitStatusDelegate::updateFromRepository(const QString &repoPath)
{
    clearStatuses();

    // Check if Git is available and if this is a Git repository
    QProcess gitCheck;
    gitCheck.setWorkingDirectory(repoPath);
    gitCheck.start("git", QStringList() << "rev-parse" << "--git-dir");
    gitCheck.waitForFinished(1000);

    if (gitCheck.exitCode() != 0) {
        // Not a Git repository
        return;
    }

    // Get Git status
    QProcess gitStatus;
    gitStatus.setWorkingDirectory(repoPath);
    gitStatus.start("git", QStringList() << "status" << "--porcelain" << "--ignored");
    gitStatus.waitForFinished(5000);

    if (gitStatus.exitCode() != 0) {
        return;
    }

    QString output = gitStatus.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        if (line.length() < 4) continue;

        // Git status format: "XY filename"
        // X = index status, Y = working tree status
        QString statusCode = line.left(2);
        QString fileName = line.mid(3);

        // Remove quotes if present
        if (fileName.startsWith('"') && fileName.endsWith('"')) {
            fileName = fileName.mid(1, fileName.length() - 2);
        }

        QString fullPath = QDir(repoPath).absoluteFilePath(fileName);

        GitStatus status = GitStatus::Clean;

        // Determine status based on status code
        if (statusCode == "??") {
            status = GitStatus::Untracked;
        } else if (statusCode == "!!") {
            status = GitStatus::Ignored;
        } else if (statusCode.contains('A')) {
            status = GitStatus::Added;
        } else if (statusCode.contains('M')) {
            status = GitStatus::Modified;
        } else if (statusCode.contains('D')) {
            status = GitStatus::Deleted;
        } else if (statusCode.contains('R')) {
            status = GitStatus::Renamed;
        } else if (statusCode.contains('U') || statusCode == "DD" || statusCode == "AA") {
            status = GitStatus::Conflicted;
        }

        setFileStatus(fullPath, status);

        // Also check parent directories and mark them as having changes
        QFileInfo fileInfo(fullPath);
        QString dirPath = fileInfo.absolutePath();
        while (dirPath.startsWith(repoPath) && dirPath != repoPath) {
            if (!m_fileStatuses.contains(dirPath)) {
                setFileStatus(dirPath, GitStatus::Modified);
            }
            QDir dir(dirPath);
            if (!dir.cdUp()) break;
            dirPath = dir.absolutePath();
        }
    }
}

QString GitStatusDelegate::getStatusSymbol(GitStatus status) const
{
    switch (status) {
    case GitStatus::Untracked:
        return "?";
    case GitStatus::Modified:
        return "M";
    case GitStatus::Added:
        return "+";
    case GitStatus::Deleted:
        return "-";
    case GitStatus::Renamed:
        return "R";
    case GitStatus::Conflicted:
        return "!";
    case GitStatus::Ignored:
        return "◌";
    case GitStatus::Clean:
    default:
        return "";
    }
}

QColor GitStatusDelegate::getStatusColor(GitStatus status) const
{
    switch (status) {
    case GitStatus::Untracked:
        return QColor(73, 190, 170);  // Teal
    case GitStatus::Modified:
        return QColor(227, 200, 0);    // Yellow
    case GitStatus::Added:
        return QColor(65, 185, 45);    // Green
    case GitStatus::Deleted:
        return QColor(229, 57, 53);    // Red
    case GitStatus::Renamed:
        return QColor(100, 170, 255);  // Blue
    case GitStatus::Conflicted:
        return QColor(255, 100, 100);  // Bright Red
    case GitStatus::Ignored:
        return QColor(100, 100, 100);  // Gray
    case GitStatus::Clean:
    default:
        return QColor(200, 200, 200);  // Light gray
    }
}

#include "gitgutter.h"

#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QTemporaryFile>

GitGutter::GitGutter(QObject *parent)
    : QObject(parent)
{
}

QString GitGutter::repoRootFor(const QString &filePath) const
{
    QProcess proc;
    proc.setWorkingDirectory(QFileInfo(filePath).absolutePath());
    proc.start("git", {"rev-parse", "--show-toplevel"});
    proc.waitForFinished(3000);
    if (proc.exitCode() != 0)
        return {};
    return QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
}

void GitGutter::updateForFile(const QString &filePath,
                              const QString &currentContent)
{
    m_filePath = filePath;
    m_changes.clear();

    if (filePath.isEmpty()) {
        emit gutterUpdated();
        return;
    }

    QString repoRoot = repoRootFor(filePath);
    if (repoRoot.isEmpty()) {
        emit gutterUpdated();
        return;
    }

    // Get relative path from repo root
    QDir repoDir(repoRoot);
    QString relPath = repoDir.relativeFilePath(filePath);

    // Run git diff against HEAD, reading from working tree.
    // Use -U0 so we get exact hunk boundaries with no context lines.
    QProcess proc;
    proc.setWorkingDirectory(repoRoot);

    if (currentContent.isEmpty()) {
        // Diff the saved file against HEAD
        proc.start("git", {"diff", "--no-color", "-U0", "HEAD", "--", relPath});
    } else {
        // Unsaved content: write to temp file and diff against HEAD version
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        if (!tmpFile.open()) return;
        tmpFile.write(currentContent.toUtf8());
        tmpFile.flush();

        // Get the HEAD version of the file
        QProcess catProc;
        catProc.setWorkingDirectory(repoRoot);
        catProc.start("git", {"show", "HEAD:" + relPath});
        catProc.waitForFinished(3000);

        if (catProc.exitCode() != 0) {
            // File is new/untracked -- all lines are Added
            m_changes.clear();
            int lineCount = currentContent.count('\n') + 1;
            GitLineChange change;
            change.type = GitLineChange::Added;
            change.startLine = 1;
            change.lineCount = lineCount;
            m_changes.append(change);
            emit gutterUpdated();
            return;
        }

        // Write HEAD version to another temp file
        QTemporaryFile headFile;
        headFile.setAutoRemove(true);
        if (!headFile.open()) return;
        headFile.write(catProc.readAllStandardOutput());
        headFile.flush();

        // Diff the two temp files
        proc.start("git", {"diff", "--no-color", "--no-index", "-U0",
                           "--", headFile.fileName(), tmpFile.fileName()});
        proc.waitForFinished(5000);
        if (proc.exitCode() != 0 && proc.exitCode() != 1) {
            emit gutterUpdated();
            return;
        }

        QString output = QString::fromUtf8(proc.readAllStandardOutput());
        parseDiff(output);
        emit gutterUpdated();
        return;
    }

    proc.waitForFinished(5000);
    if (proc.exitCode() != 0 && proc.exitCode() != 1) {
        // exitCode 1 means "differences found" which is normal
        emit gutterUpdated();
        return;
    }

    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    parseDiff(output);
    emit gutterUpdated();
}

void GitGutter::parseDiff(const QString &diffOutput)
{
    // Parse @@ -oldStart,oldCount +newStart,newCount @@ headers
    static const QRegularExpression hunkRe(
        R"(^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@)");

    const QStringList lines = diffOutput.split('\n');
    for (const QString &line : lines) {
        QRegularExpressionMatch m = hunkRe.match(line);
        if (!m.hasMatch())
            continue;

        int oldCount = m.captured(2).isEmpty() ? 1 : m.captured(2).toInt();
        int newStart = m.captured(3).toInt();
        int newCount = m.captured(4).isEmpty() ? 1 : m.captured(4).toInt();

        GitLineChange change;
        change.startLine = newStart;

        if (oldCount == 0) {
            // Pure addition
            change.type = GitLineChange::Added;
            change.lineCount = newCount;
        } else if (newCount == 0) {
            // Pure deletion (lines removed after newStart)
            change.type = GitLineChange::Deleted;
            change.lineCount = 0;
        } else {
            // Modification (old lines replaced with new lines)
            change.type = GitLineChange::Modified;
            change.lineCount = newCount;
        }

        m_changes.append(change);
    }
}

GitLineChange::Type GitGutter::lineStatus(int line) const
{
    for (const auto &c : m_changes) {
        if (c.type == GitLineChange::Deleted) {
            // Deletion markers appear at a single line position
            if (line == c.startLine)
                return GitLineChange::Deleted;
        } else {
            if (line >= c.startLine && line < c.startLine + c.lineCount)
                return c.type;
        }
    }
    return GitLineChange::Added; // fallback, caller should use hasChangeAt
}

bool GitGutter::hasChangeAt(int line) const
{
    for (const auto &c : m_changes) {
        if (c.type == GitLineChange::Deleted) {
            if (line == c.startLine)
                return true;
        } else {
            if (line >= c.startLine && line < c.startLine + c.lineCount)
                return true;
        }
    }
    return false;
}

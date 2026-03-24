#ifndef GITDIFFVIEWER_H
#define GITDIFFVIEWER_H

#include <QWidget>
#include <QTextEdit>
#include <QString>
#include <QProcess>
#include <QTreeWidget>
#include <QSplitter>
#include <QPushButton>
#include <QCheckBox>

struct DiffHunk {
    int oldStart;
    int oldCount;
    int newStart;
    int newCount;
    QString header;
    QStringList lines;
};

struct FileDiff {
    QString filePath;
    QString oldPath;
    QString status;  // A=added, M=modified, D=deleted, R=renamed
    QVector<DiffHunk> hunks;
    bool staged;
};

class GitDiffViewer : public QWidget
{
    Q_OBJECT

public:
    explicit GitDiffViewer(QWidget *parent = nullptr);
    ~GitDiffViewer() override = default;

    void setRepository(const QString &repoPath);
    void refresh();
    void showFileDiff(const QString &filePath, bool staged = false);

public slots:
    void stageFile(const QString &filePath);
    void unstageFile(const QString &filePath);
    void stageHunk(const QString &filePath, int hunkIndex);
    void unstageHunk(const QString &filePath, int hunkIndex);
    void discardChanges(const QString &filePath);
    void commit();

signals:
    void fileStaged(const QString &filePath);
    void fileUnstaged(const QString &filePath);
    void commitCreated(const QString &hash);
    void errorOccurred(const QString &error);

private:
    void setupUI();
    void updateFileList();
    void displayDiff(const FileDiff &diff);
    QVector<FileDiff> parseGitDiff(const QString &output, bool staged);
    QString runGitCommand(const QStringList &args);

    QString m_repoPath;
    QTreeWidget *m_fileTree;
    QTextEdit *m_diffView;
    QSplitter *m_splitter;

    QPushButton *m_stageButton;
    QPushButton *m_unstageButton;
    QPushButton *m_discardButton;
    QPushButton *m_commitButton;
    QPushButton *m_refreshButton;

    QVector<FileDiff> m_unstagedChanges;
    QVector<FileDiff> m_stagedChanges;
    QString m_currentFile;
    bool m_currentlyViewingStaged;
};

class GitStagingDialog : public QWidget
{
    Q_OBJECT

public:
    explicit GitStagingDialog(QWidget *parent = nullptr);
    ~GitStagingDialog() override = default;

    void setRepository(const QString &repoPath);

private slots:
    void onCommit();

private:
    void setupUI();

    GitDiffViewer *m_diffViewer;
    QLineEdit *m_commitMessage;
    QPushButton *m_commitButton;
};

#endif // GITDIFFVIEWER_H

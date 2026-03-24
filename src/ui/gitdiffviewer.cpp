#include "gitdiffviewer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QInputDialog>
#include <QHeaderView>
#include <QTextEdit>
#include <QLineEdit>
#include <QRegularExpression>
#include <QDir>

GitDiffViewer::GitDiffViewer(QWidget *parent)
    : QWidget(parent)
    , m_currentlyViewingStaged(false)
{
    setupUI();
}

void GitDiffViewer::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Toolbar
    QHBoxLayout *toolbarLayout = new QHBoxLayout;
    m_stageButton = new QPushButton(tr("Stage"));
    m_unstageButton = new QPushButton(tr("Unstage"));
    m_discardButton = new QPushButton(tr("Discard"));
    m_commitButton = new QPushButton(tr("Commit"));
    m_refreshButton = new QPushButton(tr("Refresh"));

    toolbarLayout->addWidget(m_stageButton);
    toolbarLayout->addWidget(m_unstageButton);
    toolbarLayout->addWidget(m_discardButton);
    toolbarLayout->addWidget(m_commitButton);
    toolbarLayout->addWidget(m_refreshButton);
    toolbarLayout->addStretch();

    // Splitter
    m_splitter = new QSplitter(Qt::Horizontal, this);

    // File tree
    m_fileTree = new QTreeWidget;
    m_fileTree->setHeaderLabels({tr("File"), tr("Status")});
    m_fileTree->header()->setStretchLastSection(false);
    m_fileTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);

    // Diff view
    m_diffView = new QTextEdit;
    m_diffView->setReadOnly(true);
    m_diffView->setFontFamily("Courier New");

    m_splitter->addWidget(m_fileTree);
    m_splitter->addWidget(m_diffView);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 2);

    mainLayout->addLayout(toolbarLayout);
    mainLayout->addWidget(m_splitter);

    // Connect signals
    connect(m_fileTree, &QTreeWidget::currentItemChanged,
            this, [this](QTreeWidgetItem *current, QTreeWidgetItem *) {
        if (current) {
            QString filePath = current->text(0);
            bool staged = current->parent() &&
                         current->parent()->text(0) == tr("Staged Changes");
            showFileDiff(filePath, staged);
        }
    });

    connect(m_stageButton, &QPushButton::clicked, this, [this]() {
        if (!m_currentFile.isEmpty() && !m_currentlyViewingStaged) {
            stageFile(m_currentFile);
        }
    });

    connect(m_unstageButton, &QPushButton::clicked, this, [this]() {
        if (!m_currentFile.isEmpty() && m_currentlyViewingStaged) {
            unstageFile(m_currentFile);
        }
    });

    connect(m_discardButton, &QPushButton::clicked, this, [this]() {
        if (!m_currentFile.isEmpty()) {
            auto reply = QMessageBox::question(this, tr("Discard Changes"),
                tr("Are you sure you want to discard changes to %1?").arg(m_currentFile));
            if (reply == QMessageBox::Yes) {
                discardChanges(m_currentFile);
            }
        }
    });

    connect(m_commitButton, &QPushButton::clicked, this, &GitDiffViewer::commit);
    connect(m_refreshButton, &QPushButton::clicked, this, &GitDiffViewer::refresh);
}

void GitDiffViewer::setRepository(const QString &repoPath)
{
    m_repoPath = repoPath;
    refresh();
}

void GitDiffViewer::refresh()
{
    updateFileList();
}

void GitDiffViewer::updateFileList()
{
    m_fileTree->clear();

    // Get unstaged changes
    QString unstagedDiff = runGitCommand({"diff", "--name-status"});
    m_unstagedChanges = parseGitDiff(unstagedDiff, false);

    // Get staged changes
    QString stagedDiff = runGitCommand({"diff", "--cached", "--name-status"});
    m_stagedChanges = parseGitDiff(stagedDiff, true);

    // Add to tree
    QTreeWidgetItem *unstagedRoot = new QTreeWidgetItem(m_fileTree, {tr("Unstaged Changes")});
    QTreeWidgetItem *stagedRoot = new QTreeWidgetItem(m_fileTree, {tr("Staged Changes")});

    for (const FileDiff &diff : m_unstagedChanges) {
        QString statusText;
        switch (diff.status[0].toLatin1()) {
        case 'M': statusText = "Modified"; break;
        case 'A': statusText = "Added"; break;
        case 'D': statusText = "Deleted"; break;
        case 'R': statusText = "Renamed"; break;
        default: statusText = "Unknown"; break;
        }
        new QTreeWidgetItem(unstagedRoot, {diff.filePath, statusText});
    }

    for (const FileDiff &diff : m_stagedChanges) {
        QString statusText;
        switch (diff.status[0].toLatin1()) {
        case 'M': statusText = "Modified"; break;
        case 'A': statusText = "Added"; break;
        case 'D': statusText = "Deleted"; break;
        case 'R': statusText = "Renamed"; break;
        default: statusText = "Unknown"; break;
        }
        new QTreeWidgetItem(stagedRoot, {diff.filePath, statusText});
    }

    m_fileTree->expandAll();
}

void GitDiffViewer::showFileDiff(const QString &filePath, bool staged)
{
    m_currentFile = filePath;
    m_currentlyViewingStaged = staged;

    QStringList args = {"diff"};
    if (staged) {
        args << "--cached";
    }
    args << filePath;

    QString diff = runGitCommand(args);

    // Format diff with colors
    QStringList lines = diff.split('\n');
    QString html = "<pre style='font-family: Courier New; font-size: 10pt;'>";

    for (const QString &line : lines) {
        QString color;
        if (line.startsWith("+++") || line.startsWith("---")) {
            color = "#000080"; // Dark blue
        } else if (line.startsWith("+")) {
            color = "#008000"; // Green
        } else if (line.startsWith("-")) {
            color = "#800000"; // Red
        } else if (line.startsWith("@@")) {
            color = "#0080FF"; // Cyan
        } else {
            color = "#000000"; // Black
        }

        QString escapedLine = line.toHtmlEscaped();
        html += QString("<span style='color: %1;'>%2</span>\n").arg(color, escapedLine);
    }

    html += "</pre>";
    m_diffView->setHtml(html);
}

void GitDiffViewer::stageFile(const QString &filePath)
{
    runGitCommand({"add", filePath});
    emit fileStaged(filePath);
    refresh();
}

void GitDiffViewer::unstageFile(const QString &filePath)
{
    runGitCommand({"reset", "HEAD", filePath});
    emit fileUnstaged(filePath);
    refresh();
}

void GitDiffViewer::stageHunk(const QString &filePath, int hunkIndex)
{
    // This would require interactive staging
    // For now, just stage the whole file
    stageFile(filePath);
}

void GitDiffViewer::unstageHunk(const QString &filePath, int hunkIndex)
{
    // This would require interactive staging
    // For now, just unstage the whole file
    unstageFile(filePath);
}

void GitDiffViewer::discardChanges(const QString &filePath)
{
    runGitCommand({"checkout", "--", filePath});
    refresh();
}

void GitDiffViewer::commit()
{
    bool ok;
    QString message = QInputDialog::getMultiLineText(this, tr("Commit Message"),
                                                     tr("Enter commit message:"),
                                                     QString(), &ok);

    if (!ok || message.isEmpty()) {
        return;
    }

    QString result = runGitCommand({"commit", "-m", message});

    if (result.contains("nothing to commit")) {
        QMessageBox::warning(this, tr("Nothing to Commit"),
                           tr("No staged changes to commit."));
    } else {
        // Extract commit hash
        QRegularExpression hashRegex("\\[\\w+\\s+([a-f0-9]+)\\]");
        QRegularExpressionMatch match = hashRegex.match(result);
        QString hash = match.hasMatch() ? match.captured(1) : QString();

        emit commitCreated(hash);
        QMessageBox::information(this, tr("Commit Created"),
                               tr("Successfully created commit."));
        refresh();
    }
}

QVector<FileDiff> GitDiffViewer::parseGitDiff(const QString &output, bool staged)
{
    QVector<FileDiff> diffs;
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        QStringList parts = line.split('\t');
        if (parts.size() >= 2) {
            FileDiff diff;
            diff.status = parts[0];
            diff.filePath = parts[1];
            diff.staged = staged;
            diffs.append(diff);
        }
    }

    return diffs;
}

QString GitDiffViewer::runGitCommand(const QStringList &args)
{
    QProcess process;
    process.setWorkingDirectory(m_repoPath);
    process.start("git", args);
    process.waitForFinished();

    if (process.exitCode() != 0) {
        QString error = process.readAllStandardError();
        emit errorOccurred(error);
        return QString();
    }

    return process.readAllStandardOutput();
}

// GitStagingDialog implementation
GitStagingDialog::GitStagingDialog(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void GitStagingDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_diffViewer = new GitDiffViewer(this);

    QHBoxLayout *commitLayout = new QHBoxLayout;
    QLabel *label = new QLabel(tr("Commit Message:"));
    m_commitMessage = new QLineEdit;
    m_commitButton = new QPushButton(tr("Commit"));

    commitLayout->addWidget(label);
    commitLayout->addWidget(m_commitMessage);
    commitLayout->addWidget(m_commitButton);

    mainLayout->addWidget(m_diffViewer);
    mainLayout->addLayout(commitLayout);

    connect(m_commitButton, &QPushButton::clicked, this, &GitStagingDialog::onCommit);
}

void GitStagingDialog::setRepository(const QString &repoPath)
{
    m_diffViewer->setRepository(repoPath);
}

void GitStagingDialog::onCommit()
{
    QString message = m_commitMessage->text();
    if (message.isEmpty()) {
        QMessageBox::warning(this, tr("Empty Message"),
                           tr("Please enter a commit message."));
        return;
    }

    m_diffViewer->commit();
    m_commitMessage->clear();
}

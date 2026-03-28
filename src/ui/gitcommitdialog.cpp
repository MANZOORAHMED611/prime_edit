#include "gitcommitdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFont>
#include <QFileInfo>

GitCommitDialog::GitCommitDialog(const QString &repoPath, QWidget *parent)
    : QDialog(parent)
    , m_repoPath(repoPath)
{
    setWindowTitle(tr("Git Commit"));
    setMinimumSize(700, 500);
    setupUi();
    refresh();
}

void GitCommitDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    // Branch label
    m_branchLabel = new QLabel(this);
    m_branchLabel->setStyleSheet("font-weight: bold; padding: 4px;");
    mainLayout->addWidget(m_branchLabel);

    // Splitter: file tree + diff preview
    m_splitter = new QSplitter(Qt::Horizontal, this);

    // File tree with checkboxes
    m_fileTree = new QTreeWidget(this);
    m_fileTree->setHeaderLabels({tr("Status"), tr("File")});
    m_fileTree->header()->setStretchLastSection(true);
    m_fileTree->setRootIsDecorated(false);
    m_fileTree->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_fileTree, &QTreeWidget::currentItemChanged,
            this, &GitCommitDialog::onFileSelectionChanged);
    m_splitter->addWidget(m_fileTree);

    // Diff preview
    m_diffPreview = new QPlainTextEdit(this);
    m_diffPreview->setReadOnly(true);
    QFont monoFont("Monospace", 9);
    monoFont.setStyleHint(QFont::Monospace);
    m_diffPreview->setFont(monoFont);
    m_splitter->addWidget(m_diffPreview);
    m_splitter->setSizes({250, 450});

    mainLayout->addWidget(m_splitter, 1);

    // Commit message area
    auto *msgLayout = new QVBoxLayout;
    auto *msgHeader = new QHBoxLayout;
    msgHeader->addWidget(new QLabel(tr("Commit Message:"), this));
    m_charCount = new QLabel("0", this);
    m_charCount->setStyleSheet("color: gray;");
    msgHeader->addStretch();
    msgHeader->addWidget(m_charCount);
    msgLayout->addLayout(msgHeader);

    m_commitMessage = new QTextEdit(this);
    m_commitMessage->setMaximumHeight(100);
    m_commitMessage->setPlaceholderText(
        tr("Enter commit message..."));
    connect(m_commitMessage, &QTextEdit::textChanged, this, [this]() {
        int len = m_commitMessage->toPlainText().length();
        m_charCount->setText(QString::number(len));
        // Warn if first line exceeds 72 chars
        QString firstLine = m_commitMessage->toPlainText()
                                .split('\n').first();
        if (firstLine.length() > 72) {
            m_charCount->setStyleSheet("color: orange; font-weight: bold;");
        } else {
            m_charCount->setStyleSheet("color: gray;");
        }
    });
    msgLayout->addWidget(m_commitMessage);
    mainLayout->addLayout(msgLayout);

    // Bottom row: amend checkbox + buttons
    auto *bottomLayout = new QHBoxLayout;
    m_amendCheck = new QCheckBox(tr("Amend last commit"), this);
    connect(m_amendCheck, &QCheckBox::toggled,
            this, &GitCommitDialog::onAmendToggled);
    bottomLayout->addWidget(m_amendCheck);
    bottomLayout->addStretch();

    m_cancelButton = new QPushButton(tr("Cancel"), this);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    bottomLayout->addWidget(m_cancelButton);

    m_commitButton = new QPushButton(tr("Commit"), this);
    m_commitButton->setDefault(true);
    connect(m_commitButton, &QPushButton::clicked,
            this, &GitCommitDialog::onCommitClicked);
    bottomLayout->addWidget(m_commitButton);

    mainLayout->addLayout(bottomLayout);
}

void GitCommitDialog::refresh()
{
    loadBranchName();
    loadFileList();
}

void GitCommitDialog::loadBranchName()
{
    QString branch = runGit({"rev-parse", "--abbrev-ref", "HEAD"}).trimmed();
    m_branchLabel->setText(tr("Branch: %1").arg(branch));
}

void GitCommitDialog::loadFileList()
{
    m_fileTree->clear();

    // Get status -- porcelain for easy parsing
    QString status = runGit({"status", "--porcelain", "-uall"});
    const QStringList lines = status.split('\n', Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        if (line.length() < 4) continue;

        QChar indexStatus = line.at(0);
        QChar workStatus = line.at(1);
        QString filePath = line.mid(3).trimmed();

        // Determine display status
        QString statusText;
        bool staged = false;
        if (indexStatus == 'A') {
            statusText = tr("Added");
            staged = true;
        } else if (indexStatus == 'M') {
            statusText = tr("Modified (staged)");
            staged = true;
        } else if (indexStatus == 'D') {
            statusText = tr("Deleted (staged)");
            staged = true;
        } else if (indexStatus == 'R') {
            statusText = tr("Renamed");
            staged = true;
        } else if (workStatus == 'M') {
            statusText = tr("Modified");
        } else if (workStatus == 'D') {
            statusText = tr("Deleted");
        } else if (indexStatus == '?' && workStatus == '?') {
            statusText = tr("Untracked");
        } else {
            statusText = QString(indexStatus) + QString(workStatus);
        }

        auto *item = new QTreeWidgetItem(m_fileTree);
        item->setCheckState(0, staged ? Qt::Checked : Qt::Unchecked);
        item->setText(0, statusText);
        item->setText(1, filePath);
        item->setData(0, Qt::UserRole, staged);
        item->setData(1, Qt::UserRole, filePath);
    }
}

void GitCommitDialog::onFileSelectionChanged()
{
    QTreeWidgetItem *item = m_fileTree->currentItem();
    if (!item) return;

    QString filePath = item->data(1, Qt::UserRole).toString();
    bool staged = item->checkState(0) == Qt::Checked;
    showDiffForFile(filePath, staged);
}

void GitCommitDialog::showDiffForFile(const QString &filePath, bool staged)
{
    QStringList args = {"diff", "--no-color"};
    if (staged) args << "--cached";
    args << "HEAD" << "--" << filePath;

    QString diff = runGit(args);
    if (diff.isEmpty()) {
        // Try without HEAD for untracked files
        diff = runGit({"diff", "--no-color", "--no-index",
                       "/dev/null", filePath});
    }
    m_diffPreview->setPlainText(diff);
}

void GitCommitDialog::onAmendToggled(bool checked)
{
    if (checked) {
        // Pre-fill with last commit message
        QString lastMsg = runGit({"log", "-1", "--format=%B"}).trimmed();
        m_commitMessage->setText(lastMsg);
    }
}

void GitCommitDialog::onCommitClicked()
{
    QString message = m_commitMessage->toPlainText().trimmed();
    if (message.isEmpty()) {
        QMessageBox::warning(this, tr("Commit"),
                             tr("Commit message cannot be empty."));
        return;
    }

    // Stage checked files, unstage unchecked files
    for (int i = 0; i < m_fileTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_fileTree->topLevelItem(i);
        QString filePath = item->data(1, Qt::UserRole).toString();
        if (item->checkState(0) == Qt::Checked) {
            runGit({"add", "--", filePath});
        } else {
            // Reset if it was staged
            bool wasStagedOriginally = item->data(0, Qt::UserRole).toBool();
            if (wasStagedOriginally) {
                runGit({"reset", "HEAD", "--", filePath});
            }
        }
    }

    // Check if there are staged changes
    QString stagedCheck = runGit({"diff", "--cached", "--name-only"});
    if (stagedCheck.trimmed().isEmpty() && !m_amendCheck->isChecked()) {
        QMessageBox::warning(this, tr("Commit"),
                             tr("No files staged for commit."));
        return;
    }

    // Commit
    QStringList commitArgs = {"commit", "-m", message};
    if (m_amendCheck->isChecked()) {
        commitArgs << "--amend";
    }

    QString result = runGit(commitArgs);

    // Get the new commit hash
    QString hash = runGit({"rev-parse", "--short", "HEAD"}).trimmed();

    emit commitCreated(hash, message);
    accept();
}

QString GitCommitDialog::runGit(const QStringList &args) const
{
    QProcess proc;
    proc.setWorkingDirectory(m_repoPath);
    proc.start("git", args);
    proc.waitForFinished(10000);
    return QString::fromUtf8(proc.readAllStandardOutput());
}

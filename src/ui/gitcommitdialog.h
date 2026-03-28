#ifndef GITCOMMITDIALOG_H
#define GITCOMMITDIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QProcess>
#include <QSplitter>

class GitCommitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GitCommitDialog(const QString &repoPath,
                             QWidget *parent = nullptr);

    /// Refresh the file list and branch display.
    void refresh();

signals:
    void commitCreated(const QString &hash, const QString &message);

private slots:
    void onFileSelectionChanged();
    void onCommitClicked();
    void onAmendToggled(bool checked);

private:
    void setupUi();
    void loadFileList();
    void loadBranchName();
    void showDiffForFile(const QString &filePath, bool staged);
    QString runGit(const QStringList &args) const;

    QString m_repoPath;

    // Widgets
    QLabel *m_branchLabel = nullptr;
    QTreeWidget *m_fileTree = nullptr;
    QPlainTextEdit *m_diffPreview = nullptr;
    QTextEdit *m_commitMessage = nullptr;
    QLabel *m_charCount = nullptr;
    QCheckBox *m_amendCheck = nullptr;
    QPushButton *m_commitButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QSplitter *m_splitter = nullptr;
};

#endif // GITCOMMITDIALOG_H

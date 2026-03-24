#ifndef FILEEXPLORER_H
#define FILEEXPLORER_H

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QLineEdit>
#include <QToolButton>
#include <QVBoxLayout>
#include <QLabel>

class GitStatusDelegate;

class FileExplorer : public QWidget
{
    Q_OBJECT

public:
    explicit FileExplorer(QWidget *parent = nullptr);
    ~FileExplorer() override;

    void setRootPath(const QString &path);
    QString rootPath() const;

    void setFilter(const QString &filter);

signals:
    void fileDoubleClicked(const QString &filePath);
    void fileSelected(const QString &filePath);
    void rootPathChanged(const QString &path);

private slots:
    void onItemDoubleClicked(const QModelIndex &index);
    void onItemClicked(const QModelIndex &index);
    void onOpenFolder();
    void onGoUp();
    void onRefresh();
    void onFilterChanged(const QString &text);

private:
    void setupUi();
    void updatePathLabel();

    QTreeView *m_treeView;
    QFileSystemModel *m_model;
    GitStatusDelegate *m_gitDelegate;
    QLineEdit *m_filterEdit;
    QLabel *m_pathLabel;
    QToolButton *m_openFolderBtn;
    QToolButton *m_goUpBtn;
    QToolButton *m_refreshBtn;
    QString m_rootPath;
};

#endif // FILEEXPLORER_H

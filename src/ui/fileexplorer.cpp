#include "fileexplorer.h"
#include "gitstatusdelegate.h"
#include <QFileDialog>
#include <QDir>
#include <QHeaderView>
#include <QHBoxLayout>

FileExplorer::FileExplorer(QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    // Set default root to home directory
    setRootPath(QDir::homePath());
}

FileExplorer::~FileExplorer()
{
}

void FileExplorer::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    // Toolbar
    QHBoxLayout *toolbarLayout = new QHBoxLayout();
    toolbarLayout->setContentsMargins(4, 4, 4, 4);
    toolbarLayout->setSpacing(2);

    m_openFolderBtn = new QToolButton(this);
    m_openFolderBtn->setText("📁");
    m_openFolderBtn->setToolTip(tr("Open Folder"));
    connect(m_openFolderBtn, &QToolButton::clicked, this, &FileExplorer::onOpenFolder);

    m_goUpBtn = new QToolButton(this);
    m_goUpBtn->setText("⬆");
    m_goUpBtn->setToolTip(tr("Go Up"));
    connect(m_goUpBtn, &QToolButton::clicked, this, &FileExplorer::onGoUp);

    m_refreshBtn = new QToolButton(this);
    m_refreshBtn->setText("🔄");
    m_refreshBtn->setToolTip(tr("Refresh"));
    connect(m_refreshBtn, &QToolButton::clicked, this, &FileExplorer::onRefresh);

    toolbarLayout->addWidget(m_openFolderBtn);
    toolbarLayout->addWidget(m_goUpBtn);
    toolbarLayout->addWidget(m_refreshBtn);
    toolbarLayout->addStretch();

    layout->addLayout(toolbarLayout);

    // Path label
    m_pathLabel = new QLabel(this);
    m_pathLabel->setWordWrap(true);
    m_pathLabel->setStyleSheet("QLabel { padding: 4px; background: #2d2d2d; color: #ccc; }");
    layout->addWidget(m_pathLabel);

    // Filter
    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText(tr("Filter files..."));
    m_filterEdit->setClearButtonEnabled(true);
    connect(m_filterEdit, &QLineEdit::textChanged, this, &FileExplorer::onFilterChanged);
    layout->addWidget(m_filterEdit);

    // Tree view
    m_model = new QFileSystemModel(this);
    m_model->setRootPath("");
    m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(20);
    m_treeView->setSortingEnabled(true);
    m_treeView->sortByColumn(0, Qt::AscendingOrder);

    // Set Git status delegate
    m_gitDelegate = new GitStatusDelegate(this);
    m_treeView->setItemDelegate(m_gitDelegate);

    // Hide columns except name
    m_treeView->hideColumn(1); // Size
    m_treeView->hideColumn(2); // Type
    m_treeView->hideColumn(3); // Date Modified

    m_treeView->header()->hide();
    m_treeView->setHeaderHidden(true);

    connect(m_treeView, &QTreeView::doubleClicked, this, &FileExplorer::onItemDoubleClicked);
    connect(m_treeView, &QTreeView::clicked, this, &FileExplorer::onItemClicked);

    layout->addWidget(m_treeView, 1);

    setLayout(layout);
}

void FileExplorer::setRootPath(const QString &path)
{
    m_rootPath = path;
    QModelIndex index = m_model->setRootPath(path);
    m_treeView->setRootIndex(index);
    updatePathLabel();

    // Update Git status indicators
    m_gitDelegate->updateFromRepository(path);

    emit rootPathChanged(path);
}

QString FileExplorer::rootPath() const
{
    return m_rootPath;
}

void FileExplorer::setFilter(const QString &filter)
{
    m_filterEdit->setText(filter);
}

void FileExplorer::onItemDoubleClicked(const QModelIndex &index)
{
    QString filePath = m_model->filePath(index);
    QFileInfo info(filePath);

    if (info.isDir()) {
        setRootPath(filePath);
    } else {
        emit fileDoubleClicked(filePath);
    }
}

void FileExplorer::onItemClicked(const QModelIndex &index)
{
    QString filePath = m_model->filePath(index);
    QFileInfo info(filePath);

    if (!info.isDir()) {
        emit fileSelected(filePath);
    }
}

void FileExplorer::onOpenFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Folder"), m_rootPath);
    if (!dir.isEmpty()) {
        setRootPath(dir);
    }
}

void FileExplorer::onGoUp()
{
    QDir dir(m_rootPath);
    if (dir.cdUp()) {
        setRootPath(dir.absolutePath());
    }
}

void FileExplorer::onRefresh()
{
    // Re-set root path to refresh
    QString path = m_rootPath;
    m_model->setRootPath("");
    m_model->setRootPath(path);
    m_treeView->setRootIndex(m_model->index(path));

    // Refresh Git status
    m_gitDelegate->updateFromRepository(path);
    m_treeView->viewport()->update();
}

void FileExplorer::onFilterChanged(const QString &text)
{
    if (text.isEmpty()) {
        m_model->setNameFilters(QStringList());
        m_model->setNameFilterDisables(false);
    } else {
        QStringList filters;
        filters << "*" + text + "*";
        m_model->setNameFilters(filters);
        m_model->setNameFilterDisables(false);
    }
}

void FileExplorer::updatePathLabel()
{
    QString displayPath = m_rootPath;
    // Shorten home path
    QString home = QDir::homePath();
    if (displayPath.startsWith(home)) {
        displayPath = "~" + displayPath.mid(home.length());
    }
    m_pathLabel->setText(displayPath);
}

#include "searchresultspanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

SearchResultsPanel::SearchResultsPanel(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);

    // Header row
    QHBoxLayout *headerLayout = new QHBoxLayout();
    m_headerLabel = new QLabel(tr("Search Results"), this);
    m_headerLabel->setStyleSheet("font-weight: bold;");
    headerLayout->addWidget(m_headerLabel);
    headerLayout->addStretch();

    m_clearButton = new QPushButton(tr("Clear"), this);
    m_clearButton->setFixedHeight(24);
    headerLayout->addWidget(m_clearButton);
    layout->addLayout(headerLayout);

    // Tree view
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({tr("File / Line"), tr("Text")});

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setRootIsDecorated(true);
    m_treeView->header()->setStretchLastSection(true);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    layout->addWidget(m_treeView);

    connect(m_treeView, &QTreeView::doubleClicked, this, &SearchResultsPanel::onDoubleClicked);
    connect(m_clearButton, &QPushButton::clicked, this, &SearchResultsPanel::clear);
}

void SearchResultsPanel::addResult(const SearchResult &result)
{
    QString fileKey = result.filePath.isEmpty() ? tr("(Current Document)") : result.filePath;

    QStandardItem *fileItem = m_fileItems.value(fileKey, nullptr);
    if (!fileItem) {
        fileItem = new QStandardItem(fileKey);
        fileItem->setEditable(false);
        QFont f = fileItem->font();
        f.setBold(true);
        fileItem->setFont(f);
        m_model->appendRow(fileItem);
        m_fileItems[fileKey] = fileItem;
    }

    QStandardItem *lineItem = new QStandardItem(tr("Line %1, Col %2").arg(result.line).arg(result.column));
    lineItem->setEditable(false);
    lineItem->setData(result.line, Qt::UserRole);
    lineItem->setData(result.filePath, Qt::UserRole + 1);

    QStandardItem *textItem = new QStandardItem(result.lineText.trimmed());
    textItem->setEditable(false);

    fileItem->appendRow({lineItem, textItem});
}

void SearchResultsPanel::setHeader(const QString &pattern, int hits, int files)
{
    m_headerLabel->setText(tr("Search \"%1\" — %2 hits in %3 file(s)")
                           .arg(pattern).arg(hits).arg(files));
}

void SearchResultsPanel::clear()
{
    m_model->removeRows(0, m_model->rowCount());
    m_fileItems.clear();
    m_headerLabel->setText(tr("Search Results"));
}

void SearchResultsPanel::onDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    QStandardItem *item = m_model->itemFromIndex(index.siblingAtColumn(0));
    if (!item) return;

    QVariant lineData = item->data(Qt::UserRole);
    if (lineData.isValid()) {
        QString filePath = item->data(Qt::UserRole + 1).toString();
        emit resultActivated(filePath, lineData.toInt());
    }
}

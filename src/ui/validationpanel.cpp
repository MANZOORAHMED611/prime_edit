#include "validationpanel.h"
#include <QVBoxLayout>
#include <QHeaderView>

ValidationPanel::ValidationPanel(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);

    m_headerLabel = new QLabel(tr("Document Validation"), this);
    m_headerLabel->setStyleSheet("font-weight: bold;");
    layout->addWidget(m_headerLabel);

    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels(
        {tr("Severity"), tr("Line"), tr("Rule"), tr("Message")});

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setRootIsDecorated(false);
    m_treeView->header()->setStretchLastSection(true);
    m_treeView->header()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);
    layout->addWidget(m_treeView);

    connect(m_treeView, &QTreeView::doubleClicked,
            this, &ValidationPanel::onDoubleClicked);
}

void ValidationPanel::setViolations(
    const QVector<SchemaViolation> &violations)
{
    m_model->removeRows(0, m_model->rowCount());

    int errors = 0;
    int warnings = 0;
    int infos = 0;

    for (const SchemaViolation &v : violations) {
        QString sevText;
        QColor sevColor;
        switch (v.severity) {
        case SchemaViolation::Error:
            sevText = tr("Error");
            sevColor = Qt::red;
            ++errors;
            break;
        case SchemaViolation::Warning:
            sevText = tr("Warning");
            sevColor = QColor(255, 165, 0);
            ++warnings;
            break;
        case SchemaViolation::Info:
            sevText = tr("Info");
            sevColor = Qt::blue;
            ++infos;
            break;
        }

        QStandardItem *sevItem = new QStandardItem(sevText);
        sevItem->setEditable(false);
        sevItem->setForeground(sevColor);
        sevItem->setData(v.line, Qt::UserRole);

        QStandardItem *lineItem =
            new QStandardItem(QString::number(v.line));
        lineItem->setEditable(false);

        QStandardItem *ruleItem = new QStandardItem(v.rule);
        ruleItem->setEditable(false);

        QStandardItem *msgItem = new QStandardItem(v.message);
        msgItem->setEditable(false);

        m_model->appendRow({sevItem, lineItem, ruleItem, msgItem});
    }

    if (violations.isEmpty()) {
        m_headerLabel->setText(
            tr("Document Validation - All clear"));
        m_headerLabel->setStyleSheet(
            "font-weight: bold; color: green;");
    } else {
        m_headerLabel->setText(
            tr("Document Validation - %1 violation(s) "
               "(%2 errors, %3 warnings, %4 info)")
                .arg(violations.size())
                .arg(errors)
                .arg(warnings)
                .arg(infos));
        m_headerLabel->setStyleSheet(
            "font-weight: bold; color: red;");
    }
}

void ValidationPanel::clear()
{
    m_model->removeRows(0, m_model->rowCount());
    m_headerLabel->setText(tr("Document Validation"));
    m_headerLabel->setStyleSheet("font-weight: bold;");
}

void ValidationPanel::onDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    QStandardItem *item =
        m_model->itemFromIndex(index.siblingAtColumn(0));
    if (!item) return;

    QVariant lineData = item->data(Qt::UserRole);
    if (lineData.isValid()) {
        emit violationActivated(lineData.toInt());
    }
}

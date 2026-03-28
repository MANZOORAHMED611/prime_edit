#ifndef VALIDATIONPANEL_H
#define VALIDATIONPANEL_H

#include <QWidget>
#include <QTreeView>
#include <QStandardItemModel>
#include <QLabel>
#include "core/schemavalidator.h"

class ValidationPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ValidationPanel(QWidget *parent = nullptr);

    void setViolations(
        const QVector<SchemaViolation> &violations);
    void clear();

signals:
    void violationActivated(int line);

private slots:
    void onDoubleClicked(const QModelIndex &index);

private:
    QTreeView *m_treeView;
    QStandardItemModel *m_model;
    QLabel *m_headerLabel;
};

#endif // VALIDATIONPANEL_H

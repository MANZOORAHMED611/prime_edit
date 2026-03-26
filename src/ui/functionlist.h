#ifndef FUNCTIONLIST_H
#define FUNCTIONLIST_H

#include <QWidget>
#include <QTreeView>
#include <QStandardItemModel>
#include <QTimer>

class Editor;

class FunctionListPanel : public QWidget
{
    Q_OBJECT

public:
    explicit FunctionListPanel(QWidget *parent = nullptr);

    void setEditor(Editor *editor);
    void refresh();

signals:
    void functionActivated(int line);

private slots:
    void onDoubleClicked(const QModelIndex &index);

private:
    QTreeView *m_treeView;
    QStandardItemModel *m_model;
    Editor *m_editor = nullptr;
    QTimer m_debounceTimer;
};

#endif // FUNCTIONLIST_H

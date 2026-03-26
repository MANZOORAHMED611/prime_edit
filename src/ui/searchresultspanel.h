#ifndef SEARCHRESULTSPANEL_H
#define SEARCHRESULTSPANEL_H

#include <QWidget>
#include <QTreeView>
#include <QStandardItemModel>
#include <QPushButton>
#include <QLabel>
#include <QMap>
#include "core/searchengine.h"

class SearchResultsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SearchResultsPanel(QWidget *parent = nullptr);

    void addResult(const SearchResult &result);
    void setHeader(const QString &pattern, int hits, int files);
    void clear();

signals:
    void resultActivated(const QString &filePath, int line);

private slots:
    void onDoubleClicked(const QModelIndex &index);

private:
    QTreeView *m_treeView;
    QStandardItemModel *m_model;
    QLabel *m_headerLabel;
    QPushButton *m_clearButton;

    QMap<QString, QStandardItem*> m_fileItems;
};

#endif

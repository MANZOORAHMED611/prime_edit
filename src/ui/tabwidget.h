#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <QTabWidget>
#include <QTabBar>

class TabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit TabBar(QWidget *parent = nullptr);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

signals:
    void newTabRequested();
    void closeOthersRequested(int index);
    void closeToRightRequested(int index);
    void closeAllRequested();
};

class TabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit TabWidget(QWidget *parent = nullptr);

signals:
    void newTabRequested();

private slots:
    void onCloseOthersRequested(int index);
    void onCloseToRightRequested(int index);
    void onCloseAllRequested();

private:
    TabBar *m_tabBar;
};

#endif // TABWIDGET_H

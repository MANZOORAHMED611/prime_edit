#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <QTabWidget>
#include <QTabBar>
#include <QIcon>
#include <QDesktopServices>
#include <QApplication>
#include <QClipboard>

class TabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit TabBar(QWidget *parent = nullptr);

    static QIcon iconForFile(const QString &filePath);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

signals:
    void newTabRequested();
    void closeOthersRequested(int index);
    void closeToRightRequested(int index);
    void closeToLeftRequested(int index);
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
    void onCloseToLeftRequested(int index);
    void onCloseAllRequested();

private:
    TabBar *m_tabBar;
};

#endif // TABWIDGET_H

#include "tabwidget.h"
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>

TabBar::TabBar(QWidget *parent)
    : QTabBar(parent)
{
    setTabsClosable(true);
    setMovable(true);
    setDocumentMode(true);
    setExpanding(false);
    setElideMode(Qt::ElideRight);
}

void TabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (tabAt(event->pos()) < 0) {
        emit newTabRequested();
    } else {
        QTabBar::mouseDoubleClickEvent(event);
    }
}

void TabBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        int index = tabAt(event->pos());
        if (index >= 0) {
            emit tabCloseRequested(index);
            return;
        }
    }
    QTabBar::mousePressEvent(event);
}

void TabBar::contextMenuEvent(QContextMenuEvent *event)
{
    int index = tabAt(event->pos());
    if (index < 0) {
        return;
    }

    QMenu menu(this);
    QAction *closeAction = menu.addAction(tr("Close"));
    QAction *closeOthersAction = menu.addAction(tr("Close Others"));
    QAction *closeRightAction = menu.addAction(tr("Close to the Right"));
    menu.addSeparator();
    QAction *closeAllAction = menu.addAction(tr("Close All"));

    closeOthersAction->setEnabled(count() > 1);
    closeRightAction->setEnabled(index < count() - 1);

    QAction *selected = menu.exec(event->globalPos());

    if (selected == closeAction) {
        emit tabCloseRequested(index);
    } else if (selected == closeOthersAction) {
        emit closeOthersRequested(index);
    } else if (selected == closeRightAction) {
        emit closeToRightRequested(index);
    } else if (selected == closeAllAction) {
        emit closeAllRequested();
    }
}

TabWidget::TabWidget(QWidget *parent)
    : QTabWidget(parent)
{
    m_tabBar = new TabBar(this);
    setTabBar(m_tabBar);

    connect(m_tabBar, &TabBar::newTabRequested, this, &TabWidget::newTabRequested);
    connect(m_tabBar, &TabBar::closeOthersRequested, this, &TabWidget::onCloseOthersRequested);
    connect(m_tabBar, &TabBar::closeToRightRequested, this, &TabWidget::onCloseToRightRequested);
    connect(m_tabBar, &TabBar::closeAllRequested, this, &TabWidget::onCloseAllRequested);
}

void TabWidget::onCloseOthersRequested(int index)
{
    for (int i = count() - 1; i >= 0; --i) {
        if (i != index) {
            emit tabCloseRequested(i);
        }
    }
}

void TabWidget::onCloseToRightRequested(int index)
{
    for (int i = count() - 1; i > index; --i) {
        emit tabCloseRequested(i);
    }
}

void TabWidget::onCloseAllRequested()
{
    for (int i = count() - 1; i >= 0; --i) {
        emit tabCloseRequested(i);
    }
}

#include "tabwidget.h"
#include "editor.h"
#include "core/document.h"
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QFileInfo>
#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QUrl>
#include <QDir>

TabBar::TabBar(QWidget *parent)
    : QTabBar(parent)
{
    setTabsClosable(true);
    setMovable(true);
    setDocumentMode(true);
    setExpanding(false);
    setElideMode(Qt::ElideRight);
    setUsesScrollButtons(true);
}

QIcon TabBar::iconForFile(const QString &filePath)
{
    QString ext;
    if (!filePath.isEmpty()) {
        ext = QFileInfo(filePath).suffix().toLower();
    }

    QColor color;
    if (ext == "cpp" || ext == "h" || ext == "c" || ext == "hpp") {
        color = QColor("#4285f4");
    } else if (ext == "py") {
        color = QColor("#4caf50");
    } else if (ext == "js" || ext == "ts") {
        color = QColor("#ffc107");
    } else if (ext == "html" || ext == "css") {
        color = QColor("#ff5722");
    } else if (ext == "json" || ext == "xml" ||
               ext == "yaml" || ext == "yml") {
        color = QColor("#9c27b0");
    } else if (ext == "md" || ext == "txt") {
        color = QColor("#9e9e9e");
    } else {
        color = QColor("#607d8b");
    }

    QPixmap pixmap(16, 16);
    pixmap.fill(color);

    QString label = ext.left(3).toUpper();
    if (label.isEmpty()) {
        label = "NEW";
    }

    QPainter painter(&pixmap);
    QFont font = painter.font();
    font.setPixelSize(7);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, label);
    painter.end();

    return QIcon(pixmap);
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
    QAction *closeLeftAction = menu.addAction(tr("Close to the Left"));
    menu.addSeparator();
    QAction *closeAllAction = menu.addAction(tr("Close All"));

    closeOthersAction->setEnabled(count() > 1);
    closeRightAction->setEnabled(index < count() - 1);
    closeLeftAction->setEnabled(index > 0);

    QAction *selected = menu.exec(event->globalPos());

    if (selected == closeAction) {
        emit tabCloseRequested(index);
    } else if (selected == closeOthersAction) {
        emit closeOthersRequested(index);
    } else if (selected == closeRightAction) {
        emit closeToRightRequested(index);
    } else if (selected == closeLeftAction) {
        emit closeToLeftRequested(index);
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
    connect(m_tabBar, &TabBar::closeToLeftRequested, this, &TabWidget::onCloseToLeftRequested);
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

void TabWidget::onCloseToLeftRequested(int index)
{
    for (int i = index - 1; i >= 0; --i) {
        emit tabCloseRequested(i);
    }
}

void TabWidget::onCloseAllRequested()
{
    for (int i = count() - 1; i >= 0; --i) {
        emit tabCloseRequested(i);
    }
}

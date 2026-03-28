#include "completionpopup.h"
#include "theme.h"
#include <QKeyEvent>
#include <QListWidgetItem>
#include <QFont>
#include <QFontDatabase>

CompletionPopup::CompletionPopup(QWidget *parent)
    : QListWidget(parent)
{
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setMaximumHeight(200);
    setMaximumWidth(400);
    setMinimumWidth(200);

    // Set monospace font
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(10);
    setFont(font);

    // Style — theme-aware
    auto applyThemeStyle = [this]() {
        Theme theme = ThemeManager::instance().currentTheme();
        setStyleSheet(QString(
            "QListWidget {"
            "    background-color: %1;"
            "    color: %2;"
            "    border: 1px solid %3;"
            "    outline: none;"
            "}"
            "QListWidget::item {"
            "    padding: 4px 8px;"
            "    border: none;"
            "}"
            "QListWidget::item:selected {"
            "    background-color: %4;"
            "    color: #ffffff;"
            "}"
            "QListWidget::item:hover {"
            "    background-color: %5;"
            "}"
        ).arg(theme.menuBackground.name(), theme.menuForeground.name(),
              theme.borderColor.name(), theme.accentPrimary.name(),
              theme.currentLineBackground.name()));
    };
    applyThemeStyle();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, applyThemeStyle);

    connect(this, &QListWidget::itemActivated, this, &CompletionPopup::onItemActivated);
}

void CompletionPopup::setCompletions(const QVector<CompletionItem> &items)
{
    clear();
    m_items = items;

    for (const CompletionItem &item : items) {
        QString displayText = item.label;
        if (!item.detail.isEmpty()) {
            displayText += " - " + item.detail;
        }

        QListWidgetItem *listItem = new QListWidgetItem(displayText);
        listItem->setData(Qt::UserRole, item.label);
        addItem(listItem);
    }

    if (count() > 0) {
        setCurrentRow(0);
    }
}

void CompletionPopup::setSimpleCompletions(const QVector<SimpleCompletionItem> &items)
{
    clear();
    m_items.clear();

    Theme theme = ThemeManager::instance().currentTheme();

    for (const SimpleCompletionItem &item : items) {
        QString displayText = item.label;
        if (!item.detail.isEmpty()) {
            displayText += " - " + item.detail;
        }

        QListWidgetItem *listItem = new QListWidgetItem(displayText);
        listItem->setData(Qt::UserRole, item.label);

        // Color-code by kind
        switch (item.kind) {
        case SimpleCompletionItem::Keyword:
            listItem->setForeground(theme.keyword);
            break;
        case SimpleCompletionItem::Type:
            listItem->setForeground(theme.type);
            break;
        case SimpleCompletionItem::Snippet:
            listItem->setForeground(theme.function);
            break;
        case SimpleCompletionItem::Word:
        default:
            listItem->setForeground(theme.foreground);
            break;
        }

        addItem(listItem);
    }

    if (count() > 0) {
        setCurrentRow(0);
    }
}

void CompletionPopup::showAtPosition(const QPoint &globalPos)
{
    move(globalPos);
    show();
    // Don't steal focus — editor handles keyboard navigation via its keyPressEvent
}

QString CompletionPopup::selectedCompletion() const
{
    QListWidgetItem *item = currentItem();
    if (item) {
        return item->data(Qt::UserRole).toString();
    }
    return QString();
}

void CompletionPopup::keyPressEvent(QKeyEvent *event)
{
    // Keyboard navigation is handled by the editor's keyPressEvent.
    // This handler is only reached if the popup somehow receives focus.
    switch (event->key()) {
    case Qt::Key_Escape:
        hide();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        acceptCurrent();
        break;
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
    case Qt::Key_Home:
    case Qt::Key_End:
        QListWidget::keyPressEvent(event);
        break;
    default:
        hide();
        event->ignore();
        break;
    }
}

void CompletionPopup::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    hide();
}

void CompletionPopup::onItemActivated(QListWidgetItem *item)
{
    if (item) {
        emit completionSelected(item->data(Qt::UserRole).toString());
        hide();
    }
}

void CompletionPopup::acceptCurrent()
{
    if (currentItem()) {
        emit completionSelected(selectedCompletion());
        hide();
    }
}

void CompletionPopup::selectNext()
{
    int row = currentRow();
    if (row < count() - 1) {
        setCurrentRow(row + 1);
    }
}

void CompletionPopup::selectPrevious()
{
    int row = currentRow();
    if (row > 0) {
        setCurrentRow(row - 1);
    }
}

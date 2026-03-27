#include "completionpopup.h"
#include "theme.h"
#include <QKeyEvent>
#include <QListWidgetItem>
#include <QFont>
#include <QFontDatabase>

CompletionPopup::CompletionPopup(QWidget *parent)
    : QListWidget(parent)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
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

void CompletionPopup::showAtPosition(const QPoint &globalPos)
{
    move(globalPos);
    show();
    setFocus();
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
    switch (event->key()) {
    case Qt::Key_Escape:
        hide();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (currentItem()) {
            emit completionSelected(selectedCompletion());
            hide();
        }
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
        // Pass other keys to parent (the editor)
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

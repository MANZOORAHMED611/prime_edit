#include "commandpalette.h"
#include <QVBoxLayout>
#include <QListWidgetItem>
#include <QFont>
#include <QApplication>

CommandPalette::CommandPalette(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
}

void CommandPalette::setupUI()
{
    setWindowTitle(tr("Command Palette"));
    setModal(true);
    setMinimumSize(600, 400);

    // Main layout
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Search input
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Type a command..."));
    m_searchEdit->setStyleSheet(
        "QLineEdit {"
        "    padding: 10px;"
        "    font-size: 14px;"
        "    border: none;"
        "    border-bottom: 1px solid #3a3a3a;"
        "    background: #2b2b2b;"
        "    color: #cccccc;"
        "}"
    );
    layout->addWidget(m_searchEdit);

    // Command list
    m_commandList = new QListWidget(this);
    m_commandList->setStyleSheet(
        "QListWidget {"
        "    border: none;"
        "    background: #1e1e1e;"
        "    color: #cccccc;"
        "    outline: none;"
        "}"
        "QListWidget::item {"
        "    padding: 8px 10px;"
        "    border-bottom: 1px solid #2b2b2b;"
        "}"
        "QListWidget::item:selected {"
        "    background: #094771;"
        "    color: #ffffff;"
        "}"
        "QListWidget::item:hover {"
        "    background: #2a2a2a;"
        "}"
    );
    m_commandList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    layout->addWidget(m_commandList);

    // Info label
    m_infoLabel = new QLabel(this);
    m_infoLabel->setStyleSheet(
        "QLabel {"
        "    padding: 5px 10px;"
        "    background: #2b2b2b;"
        "    color: #888888;"
        "    font-size: 11px;"
        "    border-top: 1px solid #3a3a3a;"
        "}"
    );
    m_infoLabel->setText(tr("↑↓ Navigate  ↵ Execute  Esc Close"));
    layout->addWidget(m_infoLabel);

    // Connections
    connect(m_searchEdit, &QLineEdit::textChanged, this, &CommandPalette::filterCommands);
    connect(m_commandList, &QListWidget::itemActivated, this, &CommandPalette::executeSelectedCommand);
    connect(m_commandList, &QListWidget::currentRowChanged, this, &CommandPalette::updateSelection);

    // Set dialog styling
    setStyleSheet("QDialog { background: #1e1e1e; }");
}

void CommandPalette::addAction(QAction *action, const QString &category)
{
    if (!action) return;

    QString text = action->text().remove('&'); // Remove mnemonic
    if (text.isEmpty()) return;

    CommandItem item;
    item.action = action;
    item.category = category.isEmpty() ? tr("General") : category;

    // Build display text with category
    item.displayText = item.category + ": " + text;

    // Add shortcut if available
    if (!action->shortcut().isEmpty()) {
        item.displayText += "  (" + getShortcutText(action->shortcut()) + ")";
    }

    // Create searchable lowercase text
    item.searchText = (item.category + " " + text).toLower();

    m_commands.append(item);
}

void CommandPalette::clearActions()
{
    m_commands.clear();
    m_commandList->clear();
}

void CommandPalette::showAndFocus()
{
    populateList();
    show();
    m_searchEdit->clear();
    m_searchEdit->setFocus();

    // Select first item
    if (m_commandList->count() > 0) {
        m_commandList->setCurrentRow(0);
    }
}

void CommandPalette::filterCommands(const QString &text)
{
    populateList(text);
}

void CommandPalette::populateList(const QString &filter)
{
    m_commandList->clear();

    QString searchText = filter.toLower().trimmed();

    for (const CommandItem &item : m_commands) {
        // Simple fuzzy matching - check if all search terms are present
        if (searchText.isEmpty() || item.searchText.contains(searchText)) {
            QListWidgetItem *listItem = new QListWidgetItem(item.displayText);
            listItem->setData(Qt::UserRole, QVariant::fromValue(item.action));

            // Set font
            QFont font = listItem->font();
            font.setPointSize(10);
            listItem->setFont(font);

            m_commandList->addItem(listItem);
        }
    }

    // Select first item
    if (m_commandList->count() > 0) {
        m_commandList->setCurrentRow(0);
    }

    // Update info label
    m_infoLabel->setText(tr("%1 commands  ↑↓ Navigate  ↵ Execute  Esc Close").arg(m_commandList->count()));
}

void CommandPalette::executeSelectedCommand()
{
    QListWidgetItem *item = m_commandList->currentItem();
    if (!item) return;

    QAction *action = item->data(Qt::UserRole).value<QAction*>();
    if (action && action->isEnabled()) {
        accept(); // Close dialog
        action->trigger(); // Execute action
        emit commandSelected(action);
    }
}

void CommandPalette::updateSelection(int currentRow)
{
    Q_UNUSED(currentRow);
    // Could show command description here in the future
}

void CommandPalette::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        reject();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        executeSelectedCommand();
        break;
    case Qt::Key_Down:
        if (m_searchEdit->hasFocus()) {
            m_commandList->setFocus();
            if (m_commandList->count() > 0) {
                m_commandList->setCurrentRow(0);
            }
        } else {
            QDialog::keyPressEvent(event);
        }
        break;
    case Qt::Key_Up:
        if (m_commandList->hasFocus() && m_commandList->currentRow() == 0) {
            m_searchEdit->setFocus();
        } else {
            QDialog::keyPressEvent(event);
        }
        break;
    default:
        // If typing and not focused on search, focus it
        if (!m_searchEdit->hasFocus() && event->text().length() > 0 && event->text()[0].isPrint()) {
            m_searchEdit->setFocus();
            m_searchEdit->setText(event->text());
        } else {
            QDialog::keyPressEvent(event);
        }
    }
}

QString CommandPalette::getShortcutText(const QKeySequence &sequence) const
{
    return sequence.toString(QKeySequence::NativeText);
}

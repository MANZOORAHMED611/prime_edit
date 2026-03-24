#ifndef COMMANDPALETTE_H
#define COMMANDPALETTE_H

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QAction>
#include <QVector>
#include <QKeyEvent>

class CommandPalette : public QDialog
{
    Q_OBJECT

public:
    explicit CommandPalette(QWidget *parent = nullptr);

    // Add an action to the palette
    void addAction(QAction *action, const QString &category = QString());

    // Clear all actions
    void clearActions();

    // Show the palette and focus on search
    void showAndFocus();

protected:
    void keyPressEvent(QKeyEvent *event) override;

signals:
    void commandSelected(QAction *action);

private slots:
    void filterCommands(const QString &text);
    void executeSelectedCommand();
    void updateSelection(int currentRow);

private:
    struct CommandItem {
        QAction *action;
        QString category;
        QString displayText;
        QString searchText; // Lowercase for fast searching
    };

    QLineEdit *m_searchEdit;
    QListWidget *m_commandList;
    QLabel *m_infoLabel;
    QVector<CommandItem> m_commands;

    void setupUI();
    void populateList(const QString &filter = QString());
    QString getShortcutText(const QKeySequence &sequence) const;
};

#endif // COMMANDPALETTE_H

#ifndef PLUGINDIALOG_H
#define PLUGINDIALOG_H

#include <QDialog>

class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QPushButton;

/**
 * @brief Dialog for managing PrimeEdit plugins.
 *
 * Lists discovered plugins with name, version, author, description.
 * Allows enabling/disabling and shows plugin directory paths.
 */
class PluginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PluginDialog(QWidget *parent = nullptr);

private slots:
    void onItemChanged(QTreeWidgetItem *item, int column);
    void onOpenPluginDirectory();
    void refreshPluginList();

private:
    void setupUi();

    QTreeWidget *m_pluginTree = nullptr;
    QLabel *m_dirLabel = nullptr;
    QPushButton *m_openDirButton = nullptr;
    QPushButton *m_refreshButton = nullptr;

    bool m_updating = false; // guard against recursive itemChanged
};

#endif // PLUGINDIALOG_H

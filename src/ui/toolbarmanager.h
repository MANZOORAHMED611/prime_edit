#ifndef TOOLBARMANAGER_H
#define TOOLBARMANAGER_H

#include <QObject>
#include <QToolBar>
#include <QAction>
#include <QMap>

class MainWindow;

class ToolbarManager : public QObject
{
    Q_OBJECT

public:
    explicit ToolbarManager(MainWindow *mainWindow, QObject *parent = nullptr);
    ~ToolbarManager() override = default;

    void createToolbars();
    void updateToolbarState();

    QToolBar* toolbar() const { return m_toolbar; }

    // Get actions
    QAction* action(const QString &name) const;

private:
    QAction* createAction(const QString &name, const QString &text,
                         const QString &tooltip, const QString &shortcut,
                         const char *slot, const QString &iconName = QString());

    MainWindow *m_mainWindow;
    QToolBar *m_toolbar;
    QMap<QString, QAction*> m_actions;
};

#endif // TOOLBARMANAGER_H

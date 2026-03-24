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

    // Get toolbars
    QToolBar* fileToolbar() const { return m_fileToolBar; }
    QToolBar* editToolbar() const { return m_editToolBar; }
    QToolBar* searchToolbar() const { return m_searchToolBar; }
    QToolBar* viewToolbar() const { return m_viewToolBar; }
    QToolBar* formatToolbar() const { return m_formatToolBar; }
    QToolBar* macroToolbar() const { return m_macroToolBar; }

    // Get actions
    QAction* action(const QString &name) const;

private:
    void createFileToolbar();
    void createEditToolbar();
    void createSearchToolbar();
    void createViewToolbar();
    void createFormatToolbar();
    void createMacroToolbar();

    QAction* createAction(const QString &name, const QString &text,
                         const QString &tooltip, const QString &shortcut,
                         const char *slot, const QString &iconName = QString());

    MainWindow *m_mainWindow;

    QToolBar *m_fileToolBar;
    QToolBar *m_editToolBar;
    QToolBar *m_searchToolBar;
    QToolBar *m_viewToolBar;
    QToolBar *m_formatToolBar;
    QToolBar *m_macroToolBar;

    QMap<QString, QAction*> m_actions;
};

#endif // TOOLBARMANAGER_H

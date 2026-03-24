#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QString>
#include <QPluginLoader>
#include "plugininterface.h"

class PluginManager : public QObject
{
    Q_OBJECT

public:
    static PluginManager& instance();

    // Plugin loading
    void loadPlugins();
    void loadPlugin(const QString &filePath);
    void unloadPlugin(const QString &pluginName);
    void unloadAllPlugins();

    // Plugin queries
    QVector<PluginInterface*> plugins() const;
    PluginInterface* plugin(const QString &name) const;
    QStringList pluginNames() const;
    bool hasPlugin(const QString &name) const;

    // Plugin directories
    QStringList pluginDirectories() const;
    void addPluginDirectory(const QString &dir);

    // Event broadcasting
    void broadcastFileOpened(Document *document);
    void broadcastFileSaved(Document *document);
    void broadcastFileClosed(Document *document);
    void broadcastTextChanged(Editor *editor);
    void broadcastSelectionChanged(Editor *editor);
    void broadcastCursorMoved(Editor *editor);

    // Command execution
    QStringList allCommands() const;
    bool executeCommand(const QString &command, Editor *editor);

signals:
    void pluginLoaded(const QString &name);
    void pluginUnloaded(const QString &name);
    void pluginError(const QString &name, const QString &error);

private:
    PluginManager();
    ~PluginManager() override;
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    struct PluginEntry {
        QPluginLoader *loader;
        PluginInterface *interface;
        PluginInfo info;
    };

    QMap<QString, PluginEntry> m_plugins;
    QStringList m_pluginDirs;
};

#endif // PLUGINMANAGER_H

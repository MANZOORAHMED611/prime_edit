#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QString>
#include <QPluginLoader>
#include "plugininterface.h"

/**
 * @brief Manages plugin discovery, loading, lifecycle, and event dispatch.
 *
 * Singleton — access via PluginManager::instance().
 * Scans several standard directories for shared-library plugins,
 * tracks enabled/disabled state in QSettings, and forwards
 * editor events to all enabled plugins.
 */
class PluginManager : public QObject
{
    Q_OBJECT

public:
    static PluginManager &instance();

    /// Set the EditorAPI implementation that plugins will receive.
    void setEditorAPI(EditorAPI *api);

    // Plugin loading
    void loadPlugins();
    void loadPlugin(const QString &filePath);
    void unloadPlugin(const QString &pluginName);
    void unloadAllPlugins();

    // Enable / disable (persisted in QSettings)
    bool isPluginEnabled(const QString &name) const;
    void setPluginEnabled(const QString &name, bool enabled);

    // Plugin queries
    QVector<PluginInterface *> plugins() const;
    PluginInterface *plugin(const QString &name) const;
    QStringList pluginNames() const;
    bool hasPlugin(const QString &name) const;

    /// Metadata for every discovered plugin (loaded or not).
    struct PluginEntry {
        QPluginLoader *loader = nullptr;
        PluginInterface *interface = nullptr;
        QString filePath;
        QString name;
        QString version;
        QString author;
        QString description;
        bool loaded = false;
    };

    QVector<PluginEntry> allEntries() const;

    // Plugin directories
    QStringList pluginDirectories() const;
    void addPluginDirectory(const QString &dir);

    // Event broadcasting — only dispatched to enabled + loaded plugins
    void broadcastFileOpened(const QString &path);
    void broadcastFileSaved(const QString &path);
    void broadcastFileClosed(const QString &path);
    void broadcastTextChanged();
    void broadcastSelectionChanged(const QString &text);
    void broadcastLanguageChanged(const QString &lang);

signals:
    void pluginLoaded(const QString &name);
    void pluginUnloaded(const QString &name);
    void pluginError(const QString &name, const QString &error);

private:
    PluginManager();
    ~PluginManager() override;
    PluginManager(const PluginManager &) = delete;
    PluginManager &operator=(const PluginManager &) = delete;

    QMap<QString, PluginEntry> m_plugins;
    QStringList m_pluginDirs;
    EditorAPI *m_editorAPI = nullptr;
};

#endif // PLUGINMANAGER_H

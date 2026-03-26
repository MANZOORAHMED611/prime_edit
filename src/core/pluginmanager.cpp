#include "pluginmanager.h"
#include <QDir>
#include <QDebug>
#include <QStandardPaths>
#include <QCoreApplication>

PluginManager::PluginManager()
{
    // Add default plugin directories
    QString appDir = QCoreApplication::applicationDirPath();
    m_pluginDirs.append(appDir + "/plugins");

    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    m_pluginDirs.append(configDir + "/plugins");

    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_pluginDirs.append(dataDir + "/plugins");

    // System-wide plugins
    m_pluginDirs.append("/usr/lib/olive-notepad/plugins");
    m_pluginDirs.append("/usr/share/notepad-supreme/plugins");
    m_pluginDirs.append("/usr/local/share/notepad-supreme/plugins");
}

PluginManager::~PluginManager()
{
    unloadAllPlugins();
}

PluginManager& PluginManager::instance()
{
    static PluginManager instance;
    return instance;
}

void PluginManager::loadPlugins()
{
    for (const QString &dir : m_pluginDirs) {
        QDir pluginDir(dir);
        if (!pluginDir.exists()) {
            continue;
        }

        QStringList filters;
#ifdef Q_OS_WIN
        filters << "*.dll";
#elif defined(Q_OS_MAC)
        filters << "*.dylib";
#else
        filters << "*.so";
#endif

        QStringList plugins = pluginDir.entryList(filters, QDir::Files);
        for (const QString &plugin : plugins) {
            QString fullPath = pluginDir.absoluteFilePath(plugin);
            loadPlugin(fullPath);
        }
    }
}

void PluginManager::loadPlugin(const QString &filePath)
{
    QPluginLoader *loader = new QPluginLoader(filePath);
    QObject *pluginObj = loader->instance();

    if (!pluginObj) {
        emit pluginError(filePath, loader->errorString());
        delete loader;
        return;
    }

    PluginInterface *plugin = qobject_cast<PluginInterface*>(pluginObj);
    if (!plugin) {
        emit pluginError(filePath, "Not a valid Olive Notepad plugin");
        loader->unload();
        delete loader;
        return;
    }

    PluginInfo info = plugin->info();

    // Check if already loaded
    if (m_plugins.contains(info.name)) {
        emit pluginError(filePath, "Plugin already loaded: " + info.name);
        loader->unload();
        delete loader;
        return;
    }

    // Initialize plugin
    if (!plugin->initialize(qApp)) {
        emit pluginError(filePath, "Plugin initialization failed: " + info.name);
        loader->unload();
        delete loader;
        return;
    }

    // Store plugin
    PluginEntry entry;
    entry.loader = loader;
    entry.interface = plugin;
    entry.info = info;
    m_plugins[info.name] = entry;

    emit pluginLoaded(info.name);
}

void PluginManager::unloadPlugin(const QString &pluginName)
{
    if (!m_plugins.contains(pluginName)) {
        return;
    }

    PluginEntry entry = m_plugins.take(pluginName);

    if (entry.interface) {
        entry.interface->shutdown();
    }

    if (entry.loader) {
        entry.loader->unload();
        delete entry.loader;
    }

    emit pluginUnloaded(pluginName);
}

void PluginManager::unloadAllPlugins()
{
    QStringList names = m_plugins.keys();
    for (const QString &name : names) {
        unloadPlugin(name);
    }
}

QVector<PluginInterface*> PluginManager::plugins() const
{
    QVector<PluginInterface*> result;
    for (const PluginEntry &entry : m_plugins) {
        result.append(entry.interface);
    }
    return result;
}

PluginInterface* PluginManager::plugin(const QString &name) const
{
    if (m_plugins.contains(name)) {
        return m_plugins[name].interface;
    }
    return nullptr;
}

QStringList PluginManager::pluginNames() const
{
    return m_plugins.keys();
}

bool PluginManager::hasPlugin(const QString &name) const
{
    return m_plugins.contains(name);
}

QStringList PluginManager::pluginDirectories() const
{
    return m_pluginDirs;
}

void PluginManager::addPluginDirectory(const QString &dir)
{
    if (!m_pluginDirs.contains(dir)) {
        m_pluginDirs.append(dir);
    }
}

void PluginManager::broadcastFileOpened(Document *document)
{
    for (const PluginEntry &entry : m_plugins) {
        entry.interface->onFileOpened(document);
    }
}

void PluginManager::broadcastFileSaved(Document *document)
{
    for (const PluginEntry &entry : m_plugins) {
        entry.interface->onFileSaved(document);
    }
}

void PluginManager::broadcastFileClosed(Document *document)
{
    for (const PluginEntry &entry : m_plugins) {
        entry.interface->onFileClosed(document);
    }
}

void PluginManager::broadcastTextChanged(Editor *editor)
{
    for (const PluginEntry &entry : m_plugins) {
        entry.interface->onTextChanged(editor);
    }
}

void PluginManager::broadcastSelectionChanged(Editor *editor)
{
    for (const PluginEntry &entry : m_plugins) {
        entry.interface->onSelectionChanged(editor);
    }
}

void PluginManager::broadcastCursorMoved(Editor *editor)
{
    for (const PluginEntry &entry : m_plugins) {
        entry.interface->onCursorMoved(editor);
    }
}

QStringList PluginManager::allCommands() const
{
    QStringList commands;
    for (const PluginEntry &entry : m_plugins) {
        QStringList pluginCommands = entry.interface->commands();
        for (const QString &cmd : pluginCommands) {
            commands.append(entry.info.name + "::" + cmd);
        }
    }
    return commands;
}

bool PluginManager::executeCommand(const QString &command, Editor *editor)
{
    // Command format: "PluginName::CommandName"
    QStringList parts = command.split("::");
    if (parts.size() != 2) {
        return false;
    }

    QString pluginName = parts[0];
    QString cmd = parts[1];

    if (!m_plugins.contains(pluginName)) {
        return false;
    }

    m_plugins[pluginName].interface->executeCommand(cmd, editor);
    return true;
}

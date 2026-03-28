#include "pluginmanager.h"
#include <QDir>
#include <QDebug>
#include <QSettings>
#include <QStandardPaths>
#include <QCoreApplication>

PluginManager::PluginManager()
{
    // Default plugin directories (most-specific first)
    QString appDir = QCoreApplication::applicationDirPath();
    m_pluginDirs.append(appDir + "/plugins");

    QString configDir = QStandardPaths::writableLocation(
        QStandardPaths::AppConfigLocation);
    m_pluginDirs.append(configDir + "/plugins");

    QString dataDir = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    m_pluginDirs.append(dataDir + "/plugins");

    // System-wide locations
    m_pluginDirs.append("/usr/lib/prime-edit/plugins");
    m_pluginDirs.append("/usr/share/prime-edit/plugins");
    m_pluginDirs.append("/usr/local/share/prime-edit/plugins");
}

PluginManager::~PluginManager()
{
    unloadAllPlugins();
}

PluginManager &PluginManager::instance()
{
    static PluginManager inst;
    return inst;
}

void PluginManager::setEditorAPI(EditorAPI *api)
{
    m_editorAPI = api;
}

// ----------------------------------------------------------------
// Loading / unloading
// ----------------------------------------------------------------

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

        const QStringList files = pluginDir.entryList(filters, QDir::Files);
        for (const QString &file : files) {
            loadPlugin(pluginDir.absoluteFilePath(file));
        }
    }
}

void PluginManager::loadPlugin(const QString &filePath)
{
    auto *loader = new QPluginLoader(filePath);
    QObject *pluginObj = loader->instance();

    if (!pluginObj) {
        emit pluginError(filePath, loader->errorString());
        delete loader;
        return;
    }

    auto *iface = qobject_cast<PluginInterface *>(pluginObj);
    if (!iface) {
        emit pluginError(filePath, "Not a valid PrimeEdit plugin");
        loader->unload();
        delete loader;
        return;
    }

    const QString pluginName = iface->name();

    // Duplicate guard
    if (m_plugins.contains(pluginName)) {
        emit pluginError(filePath, "Plugin already loaded: " + pluginName);
        loader->unload();
        delete loader;
        return;
    }

    // Check persisted enabled state (default: enabled)
    if (!isPluginEnabled(pluginName)) {
        // Store metadata but don't initialise
        PluginEntry entry;
        entry.loader = loader;
        entry.interface = iface;
        entry.filePath = filePath;
        entry.name = pluginName;
        entry.version = iface->version();
        entry.author = iface->author();
        entry.description = iface->description();
        entry.loaded = false;
        m_plugins[pluginName] = entry;
        return;
    }

    // Initialise with the EditorAPI
    try {
        if (!iface->initialize(m_editorAPI)) {
            emit pluginError(filePath,
                             "Plugin initialisation failed: " + pluginName);
            loader->unload();
            delete loader;
            return;
        }
    } catch (const std::exception &e) {
        qWarning() << "Plugin" << pluginName << "crashed during initialize:" << e.what();
        emit pluginError(filePath, "Plugin crashed during initialize: " + pluginName);
        loader->unload();
        delete loader;
        return;
    } catch (...) {
        qWarning() << "Plugin" << pluginName << "crashed during initialize with unknown exception";
        emit pluginError(filePath, "Plugin crashed during initialize: " + pluginName);
        loader->unload();
        delete loader;
        return;
    }

    PluginEntry entry;
    entry.loader = loader;
    entry.interface = iface;
    entry.filePath = filePath;
    entry.name = pluginName;
    entry.version = iface->version();
    entry.author = iface->author();
    entry.description = iface->description();
    entry.loaded = true;
    m_plugins[pluginName] = entry;

    emit pluginLoaded(pluginName);
}

void PluginManager::unloadPlugin(const QString &pluginName)
{
    if (!m_plugins.contains(pluginName)) {
        return;
    }

    PluginEntry entry = m_plugins.take(pluginName);

    if (entry.loaded && entry.interface) {
        try {
            entry.interface->shutdown();
        } catch (...) {
            qWarning() << "Plugin" << entry.name << "threw in shutdown";
        }
    }

    if (entry.loader) {
        entry.loader->unload();
        delete entry.loader;
    }

    emit pluginUnloaded(pluginName);
}

void PluginManager::unloadAllPlugins()
{
    const QStringList names = m_plugins.keys();
    for (const QString &name : names) {
        unloadPlugin(name);
    }
}

// ----------------------------------------------------------------
// Enable / disable persistence
// ----------------------------------------------------------------

bool PluginManager::isPluginEnabled(const QString &name) const
{
    QSettings settings;
    return settings.value("Plugins/Enabled/" + name, true).toBool();
}

void PluginManager::setPluginEnabled(const QString &name, bool enabled)
{
    QSettings settings;
    settings.setValue("Plugins/Enabled/" + name, enabled);
}

// ----------------------------------------------------------------
// Queries
// ----------------------------------------------------------------

QVector<PluginInterface *> PluginManager::plugins() const
{
    QVector<PluginInterface *> result;
    for (const PluginEntry &e : m_plugins) {
        if (e.loaded && e.interface) {
            result.append(e.interface);
        }
    }
    return result;
}

PluginInterface *PluginManager::plugin(const QString &name) const
{
    if (m_plugins.contains(name) && m_plugins[name].loaded) {
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

QVector<PluginManager::PluginEntry> PluginManager::allEntries() const
{
    QVector<PluginEntry> result;
    result.reserve(m_plugins.size());
    for (const PluginEntry &e : m_plugins) {
        result.append(e);
    }
    return result;
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

// ----------------------------------------------------------------
// Event broadcasting
// ----------------------------------------------------------------

void PluginManager::broadcastFileOpened(const QString &path)
{
    for (const PluginEntry &e : m_plugins) {
        if (e.loaded && e.interface) {
            try {
                e.interface->onFileOpened(path);
            } catch (...) {
                qWarning() << "Plugin" << e.name << "threw in onFileOpened";
            }
        }
    }
}

void PluginManager::broadcastFileSaved(const QString &path)
{
    for (const PluginEntry &e : m_plugins) {
        if (e.loaded && e.interface) {
            try {
                e.interface->onFileSaved(path);
            } catch (...) {
                qWarning() << "Plugin" << e.name << "threw in onFileSaved";
            }
        }
    }
}

void PluginManager::broadcastFileClosed(const QString &path)
{
    for (const PluginEntry &e : m_plugins) {
        if (e.loaded && e.interface) {
            try {
                e.interface->onFileClosed(path);
            } catch (...) {
                qWarning() << "Plugin" << e.name << "threw in onFileClosed";
            }
        }
    }
}

void PluginManager::broadcastTextChanged()
{
    for (const PluginEntry &e : m_plugins) {
        if (e.loaded && e.interface) {
            try {
                e.interface->onTextChanged();
            } catch (...) {
                qWarning() << "Plugin" << e.name << "threw in onTextChanged";
            }
        }
    }
}

void PluginManager::broadcastSelectionChanged(const QString &text)
{
    for (const PluginEntry &e : m_plugins) {
        if (e.loaded && e.interface) {
            try {
                e.interface->onSelectionChanged(text);
            } catch (...) {
                qWarning() << "Plugin" << e.name << "threw in onSelectionChanged";
            }
        }
    }
}

void PluginManager::broadcastLanguageChanged(const QString &lang)
{
    for (const PluginEntry &e : m_plugins) {
        if (e.loaded && e.interface) {
            try {
                e.interface->onLanguageChanged(lang);
            } catch (...) {
                qWarning() << "Plugin" << e.name << "threw in onLanguageChanged";
            }
        }
    }
}

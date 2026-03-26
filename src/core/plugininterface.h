#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <QString>
#include <QObject>
#include <QJsonObject>

class Editor;
class Document;

// Plugin metadata
struct PluginInfo {
    QString name;
    QString version;
    QString author;
    QString description;
    QString homepage;
};

// Plugin interface that all plugins must implement
class PluginInterface {
public:
    virtual ~PluginInterface() = default;

    // Plugin metadata
    virtual PluginInfo info() const = 0;

    // Lifecycle
    virtual bool initialize(QObject *app) = 0;
    virtual void shutdown() = 0;

    // Event hooks
    virtual void onFileOpened(Document *document) { Q_UNUSED(document); }
    virtual void onFileSaved(Document *document) { Q_UNUSED(document); }
    virtual void onFileClosed(Document *document) { Q_UNUSED(document); }
    virtual void onTextChanged(Editor *editor) { Q_UNUSED(editor); }
    virtual void onSelectionChanged(Editor *editor) { Q_UNUSED(editor); }
    virtual void onCursorMoved(Editor *editor) { Q_UNUSED(editor); }

    // Commands - plugins can register custom commands
    virtual QStringList commands() const { return QStringList(); }
    virtual void executeCommand(const QString &command, Editor *editor) {
        Q_UNUSED(command);
        Q_UNUSED(editor);
    }
};

// Qt plugin interface macro
#define PluginInterface_iid "com.primeedit.PluginInterface/1.0"
Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)

#endif // PLUGININTERFACE_H

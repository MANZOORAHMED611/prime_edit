#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <QString>
#include <QObject>
#include <QVariant>
#include <functional>

/**
 * @brief Abstract API surface exposed to plugins.
 *
 * Plugins receive a pointer to this interface on initialize().
 * All methods are virtual so MainWindow can provide the concrete
 * implementation without plugins depending on MainWindow directly.
 */
class EditorAPI
{
public:
    virtual ~EditorAPI() = default;

    // Editor buffer access
    virtual QString currentText() const = 0;
    virtual void setText(const QString &text) = 0;
    virtual void insertText(const QString &text) = 0;
    virtual QString selectedText() const = 0;
    virtual void replaceSelection(const QString &text) = 0;
    virtual int cursorPosition() const = 0;
    virtual void setCursorPosition(int pos) = 0;
    virtual int cursorLine() const = 0;
    virtual int cursorColumn() const = 0;

    // File access
    virtual QString currentFilePath() const = 0;
    virtual QString currentLanguage() const = 0;
    virtual void openFile(const QString &path) = 0;
    virtual void saveCurrentFile() = 0;

    // UI registration
    virtual void addMenuItem(const QString &menuPath, const QString &label,
                             std::function<void()> callback) = 0;
    virtual void addToolBarButton(const QString &iconPath, const QString &tooltip,
                                  std::function<void()> callback) = 0;
    virtual void showStatusMessage(const QString &message, int timeoutMs = 3000) = 0;

    // Plugin settings persistence
    virtual QVariant pluginSetting(const QString &key,
                                   const QVariant &defaultValue = QVariant()) const = 0;
    virtual void setPluginSetting(const QString &key, const QVariant &value) = 0;
};

/**
 * @brief Interface that every PrimeEdit plugin must implement.
 */
class PluginInterface
{
public:
    virtual ~PluginInterface() = default;

    // Metadata
    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual QString description() const = 0;
    virtual QString author() const = 0;

    // Lifecycle
    virtual bool initialize(EditorAPI *api) = 0;
    virtual void shutdown() = 0;

    // Event hooks — override the ones you care about
    virtual void onFileOpened(const QString &path) { Q_UNUSED(path); }
    virtual void onFileSaved(const QString &path) { Q_UNUSED(path); }
    virtual void onFileClosed(const QString &path) { Q_UNUSED(path); }
    virtual void onTextChanged() {}
    virtual void onSelectionChanged(const QString &text) { Q_UNUSED(text); }
    virtual void onLanguageChanged(const QString &lang) { Q_UNUSED(lang); }
};

#define PluginInterface_iid "com.primeedit.PluginInterface/1.0"
Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)

#endif // PLUGININTERFACE_H

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QFont>
#include <QColor>
#include <QVariant>

class Settings : public QObject
{
    Q_OBJECT

public:
    static Settings &instance();

    void load();
    void save();
    void reset();

    // Editor settings
    QString fontFamily() const { return m_fontFamily; }
    void setFontFamily(const QString &family);
    int fontSize() const { return m_fontSize; }
    void setFontSize(int size);
    QFont font() const;

    int tabWidth() const { return m_tabWidth; }
    void setTabWidth(int width);
    bool insertSpaces() const { return m_insertSpaces; }
    void setInsertSpaces(bool insert);

    bool wordWrap() const { return m_wordWrap; }
    void setWordWrap(bool wrap);
    bool showLineNumbers() const { return m_showLineNumbers; }
    void setShowLineNumbers(bool show);
    bool showMinimap() const { return m_showMinimap; }
    void setShowMinimap(bool show);
    bool highlightCurrentLine() const { return m_highlightCurrentLine; }
    void setHighlightCurrentLine(bool highlight);

    // Auto-save
    bool autoSave() const { return m_autoSave; }
    void setAutoSave(bool enabled);
    int autoSaveInterval() const { return m_autoSaveInterval; }
    void setAutoSaveInterval(int ms);

    // Theme
    QString theme() const { return m_theme; }
    void setTheme(const QString &theme);

    // Session
    bool restoreSession() const { return m_restoreSession; }
    void setRestoreSession(bool restore);

    // Recent files
    QStringList recentFiles() const { return m_recentFiles; }
    void setRecentFiles(const QStringList &files);

    // Files
    QString defaultEncoding() const { return m_defaultEncoding; }
    void setDefaultEncoding(const QString &encoding);
    QString defaultLineEnding() const { return m_defaultLineEnding; }
    void setDefaultLineEnding(const QString &ending);
    bool trimTrailingWhitespace() const { return m_trimTrailingWhitespace; }
    void setTrimTrailingWhitespace(bool trim);
    bool insertFinalNewline() const { return m_insertFinalNewline; }
    void setInsertFinalNewline(bool insert);

    // Generic get/set
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);

signals:
    void fontChanged();
    void tabSettingsChanged();
    void wordWrapChanged(bool enabled);
    void lineNumbersChanged(bool enabled);
    void minimapChanged(bool enabled);
    void highlightCurrentLineChanged(bool enabled);
    void themeChanged(const QString &theme);
    void autoSaveChanged(bool enabled);

private:
    explicit Settings(QObject *parent = nullptr);

    QString m_fontFamily = "DejaVu Sans Mono";
    int m_fontSize = 10;
    int m_tabWidth = 4;
    bool m_insertSpaces = true;
    bool m_wordWrap = false;
    bool m_showLineNumbers = true;
    bool m_showMinimap = true;
    bool m_highlightCurrentLine = true;
    bool m_autoSave = false;
    int m_autoSaveInterval = 1000;
    QString m_theme = "Notepad++";
    bool m_restoreSession = true;
    QStringList m_recentFiles;
    QString m_defaultEncoding = "UTF-8";
    QString m_defaultLineEnding = "LF";
    bool m_trimTrailingWhitespace = true;
    bool m_insertFinalNewline = true;

    QMap<QString, QVariant> m_customSettings;
};

#endif // SETTINGS_H

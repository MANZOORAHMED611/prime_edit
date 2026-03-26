#include "settings.h"
#include <QSettings>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

Settings &Settings::instance()
{
    static Settings instance;
    return instance;
}

Settings::Settings(QObject *parent)
    : QObject(parent)
{
}

void Settings::load()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString filePath = configPath + "/settings.json";

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) {
        return;
    }

    QJsonObject root = doc.object();

    // Editor settings
    if (root.contains("editor")) {
        QJsonObject editor = root["editor"].toObject();
        m_fontFamily = editor.value("fontFamily").toString(m_fontFamily);
        m_fontSize = editor.value("fontSize").toInt(m_fontSize);
        m_tabWidth = editor.value("tabWidth").toInt(m_tabWidth);
        m_insertSpaces = editor.value("insertSpaces").toBool(m_insertSpaces);
        m_wordWrap = editor.value("wordWrap").toBool(m_wordWrap);
        m_showLineNumbers = editor.value("lineNumbers").toBool(m_showLineNumbers);
        m_showMinimap = editor.value("minimap").toBool(m_showMinimap);
        m_highlightCurrentLine = editor.value("highlightCurrentLine").toBool(m_highlightCurrentLine);
        m_autoSave = editor.value("autoSave").toBool(m_autoSave);
        m_autoSaveInterval = editor.value("autoSaveInterval").toInt(m_autoSaveInterval);
    }

    // Theme
    if (root.contains("theme")) {
        QJsonObject theme = root["theme"].toObject();
        m_theme = theme.value("name").toString(m_theme);
    }

    // Session
    m_restoreSession = root.value("restoreSession").toBool(m_restoreSession);

    // Recent files
    if (root.contains("recentFiles")) {
        m_recentFiles.clear();
        QJsonArray recent = root["recentFiles"].toArray();
        for (const QJsonValue &val : recent) {
            m_recentFiles.append(val.toString());
        }
    }

    // Files
    if (root.contains("files")) {
        QJsonObject files = root["files"].toObject();
        m_defaultEncoding = files.value("encoding").toString(m_defaultEncoding);
        m_defaultLineEnding = files.value("lineEnding").toString(m_defaultLineEnding);
        m_trimTrailingWhitespace = files.value("trimTrailingWhitespace").toBool(m_trimTrailingWhitespace);
        m_insertFinalNewline = files.value("insertFinalNewline").toBool(m_insertFinalNewline);
    }
}

void Settings::save()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configPath);
    QString filePath = configPath + "/settings.json";

    QJsonObject root;

    // Editor settings
    QJsonObject editor;
    editor["fontFamily"] = m_fontFamily;
    editor["fontSize"] = m_fontSize;
    editor["tabWidth"] = m_tabWidth;
    editor["insertSpaces"] = m_insertSpaces;
    editor["wordWrap"] = m_wordWrap;
    editor["lineNumbers"] = m_showLineNumbers;
    editor["minimap"] = m_showMinimap;
    editor["highlightCurrentLine"] = m_highlightCurrentLine;
    editor["autoSave"] = m_autoSave;
    editor["autoSaveInterval"] = m_autoSaveInterval;
    root["editor"] = editor;

    // Theme
    QJsonObject theme;
    theme["name"] = m_theme;
    root["theme"] = theme;

    // Session
    root["restoreSession"] = m_restoreSession;

    // Recent files
    QJsonArray recent;
    for (const QString &file : m_recentFiles) {
        recent.append(file);
    }
    root["recentFiles"] = recent;

    // Files
    QJsonObject files;
    files["encoding"] = m_defaultEncoding;
    files["lineEnding"] = m_defaultLineEnding;
    files["trimTrailingWhitespace"] = m_trimTrailingWhitespace;
    files["insertFinalNewline"] = m_insertFinalNewline;
    root["files"] = files;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
}

void Settings::reset()
{
    m_fontFamily = "DejaVu Sans Mono";
    m_fontSize = 10;
    m_tabWidth = 4;
    m_insertSpaces = true;
    m_wordWrap = false;
    m_showLineNumbers = true;
    m_showMinimap = true;
    m_highlightCurrentLine = true;
    m_autoSave = false;
    m_autoSaveInterval = 1000;
    m_theme = "Notepad++";
    m_restoreSession = true;
    m_defaultEncoding = "UTF-8";
    m_defaultLineEnding = "LF";
    m_trimTrailingWhitespace = true;
    m_insertFinalNewline = true;

    emit fontChanged();
    emit tabSettingsChanged();
    emit wordWrapChanged(m_wordWrap);
    emit lineNumbersChanged(m_showLineNumbers);
    emit minimapChanged(m_showMinimap);
    emit highlightCurrentLineChanged(m_highlightCurrentLine);
    emit themeChanged(m_theme);
}

void Settings::setFontFamily(const QString &family)
{
    if (m_fontFamily != family) {
        m_fontFamily = family;
        emit fontChanged();
    }
}

void Settings::setFontSize(int size)
{
    if (m_fontSize != size) {
        m_fontSize = size;
        emit fontChanged();
    }
}

QFont Settings::font() const
{
    QFont f(m_fontFamily, m_fontSize);
    f.setStyleHint(QFont::Monospace);
    return f;
}

void Settings::setTabWidth(int width)
{
    if (m_tabWidth != width) {
        m_tabWidth = width;
        emit tabSettingsChanged();
    }
}

void Settings::setInsertSpaces(bool insert)
{
    if (m_insertSpaces != insert) {
        m_insertSpaces = insert;
        emit tabSettingsChanged();
    }
}

void Settings::setWordWrap(bool wrap)
{
    if (m_wordWrap != wrap) {
        m_wordWrap = wrap;
        emit wordWrapChanged(m_wordWrap);
    }
}

void Settings::setShowLineNumbers(bool show)
{
    if (m_showLineNumbers != show) {
        m_showLineNumbers = show;
        emit lineNumbersChanged(m_showLineNumbers);
    }
}

void Settings::setShowMinimap(bool show)
{
    if (m_showMinimap != show) {
        m_showMinimap = show;
        emit minimapChanged(m_showMinimap);
    }
}

void Settings::setHighlightCurrentLine(bool highlight)
{
    if (m_highlightCurrentLine != highlight) {
        m_highlightCurrentLine = highlight;
        emit highlightCurrentLineChanged(m_highlightCurrentLine);
    }
}

void Settings::setAutoSave(bool enabled)
{
    if (m_autoSave != enabled) {
        m_autoSave = enabled;
        emit autoSaveChanged(m_autoSave);
    }
}

void Settings::setAutoSaveInterval(int ms)
{
    m_autoSaveInterval = ms;
}

void Settings::setTheme(const QString &theme)
{
    if (m_theme != theme) {
        m_theme = theme;
        emit themeChanged(m_theme);
    }
}

void Settings::setRestoreSession(bool restore)
{
    m_restoreSession = restore;
}

void Settings::setRecentFiles(const QStringList &files)
{
    m_recentFiles = files;
}

void Settings::setDefaultEncoding(const QString &encoding)
{
    m_defaultEncoding = encoding;
}

void Settings::setDefaultLineEnding(const QString &ending)
{
    m_defaultLineEnding = ending;
}

void Settings::setTrimTrailingWhitespace(bool trim)
{
    m_trimTrailingWhitespace = trim;
}

void Settings::setInsertFinalNewline(bool insert)
{
    m_insertFinalNewline = insert;
}

QVariant Settings::value(const QString &key, const QVariant &defaultValue) const
{
    return m_customSettings.value(key, defaultValue);
}

void Settings::setValue(const QString &key, const QVariant &value)
{
    m_customSettings[key] = value;
}

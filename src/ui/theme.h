#ifndef THEME_H
#define THEME_H

#include <QString>
#include <QColor>
#include <QFont>
#include <QJsonObject>
#include <QMap>

struct Theme {
    QString name;
    QString author;
    bool isDark;

    // Editor colors
    QColor background;
    QColor foreground;
    QColor selectionBackground;
    QColor selectionForeground;
    QColor currentLineBackground;
    QColor lineNumberForeground;
    QColor lineNumberBackground;

    // Syntax colors
    QColor keyword;
    QColor string;
    QColor comment;
    QColor number;
    QColor function;
    QColor type;
    QColor variable;
    QColor operator_;

    // UI colors
    QColor menuBackground;
    QColor menuForeground;
    QColor toolbarBackground;
    QColor statusBarBackground;
    QColor statusBarForeground;
    QColor tabActiveBackground;
    QColor tabInactiveBackground;
    QColor tabForeground;

    // Accent colors
    QColor accentPrimary;
    QColor accentSecondary;
    QColor borderColor;

    // Diagnostic colors
    QColor errorForeground;
    QColor warningForeground;
    QColor infoForeground;
    QColor hintForeground;

    // Convert to/from JSON
    QJsonObject toJson() const;
    static Theme fromJson(const QJsonObject &json);

    // Generate stylesheet
    QString toStyleSheet() const;

    // Built-in themes
    static Theme defaultLight();
    static Theme defaultDark();
    static Theme olive();
    static Theme solarizedLight();
    static Theme solarizedDark();
    static Theme monokai();
    static Theme dracula();
    static Theme nord();
};

class ThemeManager {
public:
    static ThemeManager& instance();

    // Theme management
    void loadThemes();
    void saveTheme(const Theme &theme);
    bool deleteTheme(const QString &name);

    // Get themes
    QVector<Theme> themes() const;
    Theme theme(const QString &name) const;
    Theme currentTheme() const;

    // Apply theme
    void applyTheme(const QString &name);
    void applyTheme(const Theme &theme);

    // Import/export
    bool importTheme(const QString &filePath);
    bool exportTheme(const QString &name, const QString &filePath);

private:
    ThemeManager();
    ~ThemeManager();
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    QString themesDirectory() const;
    void loadBuiltInThemes();

    QMap<QString, Theme> m_themes;
    Theme m_currentTheme;
};

#endif // THEME_H

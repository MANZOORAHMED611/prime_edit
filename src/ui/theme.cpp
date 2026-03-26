#include "theme.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QApplication>

// Theme JSON conversion
QJsonObject Theme::toJson() const
{
    QJsonObject obj;
    obj["name"] = name;
    obj["author"] = author;
    obj["isDark"] = isDark;

    // Helper lambda to convert QColor to string
    auto colorToString = [](const QColor &c) { return c.name(); };

    // Editor colors
    obj["background"] = colorToString(background);
    obj["foreground"] = colorToString(foreground);
    obj["selectionBackground"] = colorToString(selectionBackground);
    obj["selectionForeground"] = colorToString(selectionForeground);
    obj["currentLineBackground"] = colorToString(currentLineBackground);
    obj["lineNumberForeground"] = colorToString(lineNumberForeground);
    obj["lineNumberBackground"] = colorToString(lineNumberBackground);

    // Syntax colors
    obj["keyword"] = colorToString(keyword);
    obj["string"] = colorToString(string);
    obj["comment"] = colorToString(comment);
    obj["number"] = colorToString(number);
    obj["function"] = colorToString(function);
    obj["type"] = colorToString(type);
    obj["variable"] = colorToString(variable);
    obj["operator"] = colorToString(operator_);

    // UI colors
    obj["menuBackground"] = colorToString(menuBackground);
    obj["menuForeground"] = colorToString(menuForeground);
    obj["toolbarBackground"] = colorToString(toolbarBackground);
    obj["statusBarBackground"] = colorToString(statusBarBackground);
    obj["statusBarForeground"] = colorToString(statusBarForeground);
    obj["tabActiveBackground"] = colorToString(tabActiveBackground);
    obj["tabInactiveBackground"] = colorToString(tabInactiveBackground);
    obj["tabForeground"] = colorToString(tabForeground);

    // Accent colors
    obj["accentPrimary"] = colorToString(accentPrimary);
    obj["accentSecondary"] = colorToString(accentSecondary);
    obj["borderColor"] = colorToString(borderColor);

    // Gutter colors
    obj["foldMarginBackground"] = colorToString(foldMarginBackground);
    obj["bookmarkMarginBackground"] = colorToString(bookmarkMarginBackground);

    // Editor feature colors
    obj["indentGuideColor"] = colorToString(indentGuideColor);
    obj["whitespaceColor"] = colorToString(whitespaceColor);
    obj["markHighlightColor"] = colorToString(markHighlightColor);
    obj["bracketMatchBackground"] = colorToString(bracketMatchBackground);
    obj["bracketErrorBackground"] = colorToString(bracketErrorBackground);

    // Diagnostic colors
    obj["errorForeground"] = colorToString(errorForeground);
    obj["warningForeground"] = colorToString(warningForeground);
    obj["infoForeground"] = colorToString(infoForeground);
    obj["hintForeground"] = colorToString(hintForeground);

    return obj;
}

Theme Theme::fromJson(const QJsonObject &json)
{
    Theme theme;
    theme.name = json["name"].toString();
    theme.author = json["author"].toString();
    theme.isDark = json["isDark"].toBool();

    // Helper lambda
    auto stringToColor = [](const QString &s) { return QColor(s); };

    theme.background = stringToColor(json["background"].toString());
    theme.foreground = stringToColor(json["foreground"].toString());
    theme.selectionBackground = stringToColor(json["selectionBackground"].toString());
    theme.selectionForeground = stringToColor(json["selectionForeground"].toString());
    theme.currentLineBackground = stringToColor(json["currentLineBackground"].toString());
    theme.lineNumberForeground = stringToColor(json["lineNumberForeground"].toString());
    theme.lineNumberBackground = stringToColor(json["lineNumberBackground"].toString());

    theme.keyword = stringToColor(json["keyword"].toString());
    theme.string = stringToColor(json["string"].toString());
    theme.comment = stringToColor(json["comment"].toString());
    theme.number = stringToColor(json["number"].toString());
    theme.function = stringToColor(json["function"].toString());
    theme.type = stringToColor(json["type"].toString());
    theme.variable = stringToColor(json["variable"].toString());
    theme.operator_ = stringToColor(json["operator"].toString());

    theme.menuBackground = stringToColor(json["menuBackground"].toString());
    theme.menuForeground = stringToColor(json["menuForeground"].toString());
    theme.toolbarBackground = stringToColor(json["toolbarBackground"].toString());
    theme.statusBarBackground = stringToColor(json["statusBarBackground"].toString());
    theme.statusBarForeground = stringToColor(json["statusBarForeground"].toString());
    theme.tabActiveBackground = stringToColor(json["tabActiveBackground"].toString());
    theme.tabInactiveBackground = stringToColor(json["tabInactiveBackground"].toString());
    theme.tabForeground = stringToColor(json["tabForeground"].toString());

    theme.accentPrimary = stringToColor(json["accentPrimary"].toString());
    theme.accentSecondary = stringToColor(json["accentSecondary"].toString());
    theme.borderColor = stringToColor(json["borderColor"].toString());

    theme.foldMarginBackground = stringToColor(json["foldMarginBackground"].toString());
    theme.bookmarkMarginBackground = stringToColor(json["bookmarkMarginBackground"].toString());

    theme.indentGuideColor = stringToColor(json["indentGuideColor"].toString());
    theme.whitespaceColor = stringToColor(json["whitespaceColor"].toString());
    theme.markHighlightColor = stringToColor(json["markHighlightColor"].toString());
    theme.bracketMatchBackground = stringToColor(json["bracketMatchBackground"].toString());
    theme.bracketErrorBackground = stringToColor(json["bracketErrorBackground"].toString());

    theme.errorForeground = stringToColor(json["errorForeground"].toString());
    theme.warningForeground = stringToColor(json["warningForeground"].toString());
    theme.infoForeground = stringToColor(json["infoForeground"].toString());
    theme.hintForeground = stringToColor(json["hintForeground"].toString());

    return theme;
}

QString Theme::toStyleSheet() const
{
    QString qss;
    qss += QString("QMainWindow { background-color: %1; }\n").arg(background.name());
    qss += QString("QMenuBar { background-color: %1; color: %2; }\n")
        .arg(menuBackground.name(), menuForeground.name());
    qss += QString("QMenu { background-color: %1; color: %2; }\n")
        .arg(menuBackground.name(), menuForeground.name());
    qss += QString("QToolBar { background-color: %1; border: none; }\n")
        .arg(toolbarBackground.name());
    qss += QString("QStatusBar { background-color: %1; color: %2; }\n")
        .arg(statusBarBackground.name(), statusBarForeground.name());
    qss += QString("QTabBar::tab { background-color: %1; color: %2; }\n")
        .arg(tabInactiveBackground.name(), tabForeground.name());
    qss += QString("QTabBar::tab:selected { background-color: %1; }\n")
        .arg(tabActiveBackground.name());
    return qss;
}

// Built-in themes
Theme Theme::defaultDark()
{
    Theme t;
    t.name = "Default Dark";
    t.author = "Olive Notepad";
    t.isDark = true;

    t.background = QColor("#1e1e1e");
    t.foreground = QColor("#d4d4d4");
    t.selectionBackground = QColor("#264f78");
    t.selectionForeground = QColor("#ffffff");
    t.currentLineBackground = QColor("#2a2a2a");
    t.lineNumberForeground = QColor("#858585");
    t.lineNumberBackground = QColor("#1e1e1e");

    t.keyword = QColor("#569cd6");
    t.string = QColor("#ce9178");
    t.comment = QColor("#6a9955");
    t.number = QColor("#b5cea8");
    t.function = QColor("#dcdcaa");
    t.type = QColor("#4ec9b0");
    t.variable = QColor("#9cdcfe");
    t.operator_ = QColor("#d4d4d4");

    t.menuBackground = QColor("#2d2d2d");
    t.menuForeground = QColor("#cccccc");
    t.toolbarBackground = QColor("#2d2d2d");
    t.statusBarBackground = QColor("#007acc");
    t.statusBarForeground = QColor("#ffffff");
    t.tabActiveBackground = QColor("#1e1e1e");
    t.tabInactiveBackground = QColor("#2d2d2d");
    t.tabForeground = QColor("#cccccc");

    t.accentPrimary = QColor("#007acc");
    t.accentSecondary = QColor("#00a0ff");
    t.borderColor = QColor("#3e3e3e");

    t.foldMarginBackground = QColor("#2d2d2d");
    t.bookmarkMarginBackground = QColor("#1e1e1e");
    t.indentGuideColor = QColor("#404040");
    t.whitespaceColor = QColor("#3b3b3b");
    t.markHighlightColor = QColor("#613214");
    t.bracketMatchBackground = QColor("#0d5e0d");
    t.bracketErrorBackground = QColor("#5e0d0d");

    t.errorForeground = QColor("#f48771");
    t.warningForeground = QColor("#cca700");
    t.infoForeground = QColor("#75beff");
    t.hintForeground = QColor("#eeeeee");

    return t;
}

Theme Theme::defaultLight()
{
    Theme t;
    t.name = "Default Light";
    t.author = "Olive Notepad";
    t.isDark = false;

    t.background = QColor("#ffffff");
    t.foreground = QColor("#000000");
    t.selectionBackground = QColor("#add6ff");
    t.selectionForeground = QColor("#000000");
    t.currentLineBackground = QColor("#f0f0f0");
    t.lineNumberForeground = QColor("#237893");
    t.lineNumberBackground = QColor("#ffffff");

    t.keyword = QColor("#0000ff");
    t.string = QColor("#a31515");
    t.comment = QColor("#008000");
    t.number = QColor("#098658");
    t.function = QColor("#795e26");
    t.type = QColor("#267f99");
    t.variable = QColor("#001080");
    t.operator_ = QColor("#000000");

    t.menuBackground = QColor("#f3f3f3");
    t.menuForeground = QColor("#000000");
    t.toolbarBackground = QColor("#f3f3f3");
    t.statusBarBackground = QColor("#007acc");
    t.statusBarForeground = QColor("#ffffff");
    t.tabActiveBackground = QColor("#ffffff");
    t.tabInactiveBackground = QColor("#ececec");
    t.tabForeground = QColor("#333333");

    t.accentPrimary = QColor("#007acc");
    t.accentSecondary = QColor("#0098ff");
    t.borderColor = QColor("#d4d4d4");

    t.foldMarginBackground = QColor("#f3f3f3");
    t.bookmarkMarginBackground = QColor("#ffffff");
    t.indentGuideColor = QColor("#d0d0d0");
    t.whitespaceColor = QColor("#c8c8c8");
    t.markHighlightColor = QColor("#ffff00");
    t.bracketMatchBackground = QColor("#90ee90");
    t.bracketErrorBackground = QColor("#ff6666");

    t.errorForeground = QColor("#e51400");
    t.warningForeground = QColor("#bf8803");
    t.infoForeground = QColor("#1a85ff");
    t.hintForeground = QColor("#6c6c6c");

    return t;
}

Theme Theme::olive()
{
    Theme t;
    t.name = "Olive";
    t.author = "Olive Notepad";
    t.isDark = false;

    t.background = QColor("#f8f9f5");
    t.foreground = QColor("#3d4a2c");
    t.selectionBackground = QColor("#c8d5a8");
    t.selectionForeground = QColor("#2a3518");
    t.currentLineBackground = QColor("#eef2e6");
    t.lineNumberForeground = QColor("#7a8f5c");
    t.lineNumberBackground = QColor("#f3f6ec");

    t.keyword = QColor("#556b2f");
    t.string = QColor("#9b6b3f");
    t.comment = QColor("#7a8f5c");
    t.number = QColor("#6b8e23");
    t.function = QColor("#8b7355");
    t.type = QColor("#6b7c58");
    t.variable = QColor("#4a5a3a");
    t.operator_ = QColor("#3d4a2c");

    t.menuBackground = QColor("#eef2e6");
    t.menuForeground = QColor("#3d4a2c");
    t.toolbarBackground = QColor("#dce5c8");
    t.statusBarBackground = QColor("#6b8e23");
    t.statusBarForeground = QColor("#ffffff");
    t.tabActiveBackground = QColor("#f8f9f5");
    t.tabInactiveBackground = QColor("#e5e9dc");
    t.tabForeground = QColor("#3d4a2c");

    t.accentPrimary = QColor("#6b8e23");
    t.accentSecondary = QColor("#8fbc8f");
    t.borderColor = QColor("#b8c89f");

    t.foldMarginBackground = QColor("#eef2e6");
    t.bookmarkMarginBackground = QColor("#f3f6ec");
    t.indentGuideColor = QColor("#c8d5a8");
    t.whitespaceColor = QColor("#b8c89f");
    t.markHighlightColor = QColor("#e6db74");
    t.bracketMatchBackground = QColor("#a8d98a");
    t.bracketErrorBackground = QColor("#d4765b");

    t.errorForeground = QColor("#a0522d");
    t.warningForeground = QColor("#cd853f");
    t.infoForeground = QColor("#6b8e23");
    t.hintForeground = QColor("#7a8f5c");

    return t;
}

Theme Theme::monokai()
{
    Theme t;
    t.name = "Monokai";
    t.author = "Wimer Hazenberg";
    t.isDark = true;

    t.background = QColor("#272822");
    t.foreground = QColor("#f8f8f2");
    t.selectionBackground = QColor("#49483e");
    t.selectionForeground = QColor("#f8f8f2");
    t.currentLineBackground = QColor("#3e3d32");
    t.lineNumberForeground = QColor("#90908a");
    t.lineNumberBackground = QColor("#272822");

    t.keyword = QColor("#f92672");
    t.string = QColor("#e6db74");
    t.comment = QColor("#75715e");
    t.number = QColor("#ae81ff");
    t.function = QColor("#a6e22e");
    t.type = QColor("#66d9ef");
    t.variable = QColor("#f8f8f2");
    t.operator_ = QColor("#f92672");

    t.menuBackground = QColor("#2d2d2d");
    t.menuForeground = QColor("#f8f8f2");
    t.toolbarBackground = QColor("#2d2d2d");
    t.statusBarBackground = QColor("#1e1f1c");
    t.statusBarForeground = QColor("#f8f8f2");
    t.tabActiveBackground = QColor("#272822");
    t.tabInactiveBackground = QColor("#3e3d32");
    t.tabForeground = QColor("#f8f8f2");

    t.accentPrimary = QColor("#f92672");
    t.accentSecondary = QColor("#a6e22e");
    t.borderColor = QColor("#49483e");

    t.foldMarginBackground = QColor("#2d2d2d");
    t.bookmarkMarginBackground = QColor("#272822");
    t.indentGuideColor = QColor("#464741");
    t.whitespaceColor = QColor("#3e3d32");
    t.markHighlightColor = QColor("#544f19");
    t.bracketMatchBackground = QColor("#2e5e1a");
    t.bracketErrorBackground = QColor("#6e1a1a");

    t.errorForeground = QColor("#f92672");
    t.warningForeground = QColor("#e6db74");
    t.infoForeground = QColor("#66d9ef");
    t.hintForeground = QColor("#75715e");

    return t;
}

Theme Theme::dracula()
{
    Theme t;
    t.name = "Dracula";
    t.author = "Zeno Rocha";
    t.isDark = true;

    t.background = QColor("#282a36");
    t.foreground = QColor("#f8f8f2");
    t.selectionBackground = QColor("#44475a");
    t.selectionForeground = QColor("#f8f8f2");
    t.currentLineBackground = QColor("#44475a");
    t.lineNumberForeground = QColor("#6272a4");
    t.lineNumberBackground = QColor("#282a36");

    t.keyword = QColor("#ff79c6");
    t.string = QColor("#f1fa8c");
    t.comment = QColor("#6272a4");
    t.number = QColor("#bd93f9");
    t.function = QColor("#50fa7b");
    t.type = QColor("#8be9fd");
    t.variable = QColor("#f8f8f2");
    t.operator_ = QColor("#ff79c6");

    t.menuBackground = QColor("#21222c");
    t.menuForeground = QColor("#f8f8f2");
    t.toolbarBackground = QColor("#21222c");
    t.statusBarBackground = QColor("#191a21");
    t.statusBarForeground = QColor("#f8f8f2");
    t.tabActiveBackground = QColor("#282a36");
    t.tabInactiveBackground = QColor("#21222c");
    t.tabForeground = QColor("#f8f8f2");

    t.accentPrimary = QColor("#ff79c6");
    t.accentSecondary = QColor("#bd93f9");
    t.borderColor = QColor("#44475a");

    t.foldMarginBackground = QColor("#21222c");
    t.bookmarkMarginBackground = QColor("#282a36");
    t.indentGuideColor = QColor("#44475a");
    t.whitespaceColor = QColor("#3b3e50");
    t.markHighlightColor = QColor("#4a4520");
    t.bracketMatchBackground = QColor("#1a5e2a");
    t.bracketErrorBackground = QColor("#5e1a1a");

    t.errorForeground = QColor("#ff5555");
    t.warningForeground = QColor("#ffb86c");
    t.infoForeground = QColor("#8be9fd");
    t.hintForeground = QColor("#6272a4");

    return t;
}

Theme Theme::nord()
{
    Theme t;
    t.name = "Nord";
    t.author = "Arctic Ice Studio";
    t.isDark = true;

    t.background = QColor("#2e3440");
    t.foreground = QColor("#d8dee9");
    t.selectionBackground = QColor("#434c5e");
    t.selectionForeground = QColor("#d8dee9");
    t.currentLineBackground = QColor("#3b4252");
    t.lineNumberForeground = QColor("#4c566a");
    t.lineNumberBackground = QColor("#2e3440");

    t.keyword = QColor("#81a1c1");
    t.string = QColor("#a3be8c");
    t.comment = QColor("#616e88");
    t.number = QColor("#b48ead");
    t.function = QColor("#88c0d0");
    t.type = QColor("#8fbcbb");
    t.variable = QColor("#d8dee9");
    t.operator_ = QColor("#81a1c1");

    t.menuBackground = QColor("#3b4252");
    t.menuForeground = QColor("#d8dee9");
    t.toolbarBackground = QColor("#3b4252");
    t.statusBarBackground = QColor("#434c5e");
    t.statusBarForeground = QColor("#eceff4");
    t.tabActiveBackground = QColor("#2e3440");
    t.tabInactiveBackground = QColor("#3b4252");
    t.tabForeground = QColor("#d8dee9");

    t.accentPrimary = QColor("#88c0d0");
    t.accentSecondary = QColor("#81a1c1");
    t.borderColor = QColor("#434c5e");

    t.foldMarginBackground = QColor("#3b4252");
    t.bookmarkMarginBackground = QColor("#2e3440");
    t.indentGuideColor = QColor("#434c5e");
    t.whitespaceColor = QColor("#3b4252");
    t.markHighlightColor = QColor("#4a4520");
    t.bracketMatchBackground = QColor("#1a5e3a");
    t.bracketErrorBackground = QColor("#5e2a2a");

    t.errorForeground = QColor("#bf616a");
    t.warningForeground = QColor("#ebcb8b");
    t.infoForeground = QColor("#88c0d0");
    t.hintForeground = QColor("#4c566a");

    return t;
}

Theme Theme::solarizedDark()
{
    Theme t;
    t.name = "Solarized Dark";
    t.author = "Ethan Schoonover";
    t.isDark = true;

    t.background = QColor("#002b36");
    t.foreground = QColor("#839496");
    t.selectionBackground = QColor("#073642");
    t.selectionForeground = QColor("#93a1a1");
    t.currentLineBackground = QColor("#073642");
    t.lineNumberForeground = QColor("#586e75");
    t.lineNumberBackground = QColor("#002b36");

    t.keyword = QColor("#859900");
    t.string = QColor("#2aa198");
    t.comment = QColor("#586e75");
    t.number = QColor("#d33682");
    t.function = QColor("#268bd2");
    t.type = QColor("#b58900");
    t.variable = QColor("#839496");
    t.operator_ = QColor("#859900");

    t.menuBackground = QColor("#073642");
    t.menuForeground = QColor("#839496");
    t.toolbarBackground = QColor("#073642");
    t.statusBarBackground = QColor("#073642");
    t.statusBarForeground = QColor("#93a1a1");
    t.tabActiveBackground = QColor("#002b36");
    t.tabInactiveBackground = QColor("#073642");
    t.tabForeground = QColor("#839496");

    t.accentPrimary = QColor("#268bd2");
    t.accentSecondary = QColor("#2aa198");
    t.borderColor = QColor("#073642");

    t.foldMarginBackground = QColor("#073642");
    t.bookmarkMarginBackground = QColor("#002b36");
    t.indentGuideColor = QColor("#0a4a5a");
    t.whitespaceColor = QColor("#073642");
    t.markHighlightColor = QColor("#3a4a00");
    t.bracketMatchBackground = QColor("#0a4a2a");
    t.bracketErrorBackground = QColor("#5e1a1a");

    t.errorForeground = QColor("#dc322f");
    t.warningForeground = QColor("#b58900");
    t.infoForeground = QColor("#268bd2");
    t.hintForeground = QColor("#586e75");

    return t;
}

Theme Theme::solarizedLight()
{
    Theme t;
    t.name = "Solarized Light";
    t.author = "Ethan Schoonover";
    t.isDark = false;

    t.background = QColor("#fdf6e3");
    t.foreground = QColor("#657b83");
    t.selectionBackground = QColor("#eee8d5");
    t.selectionForeground = QColor("#586e75");
    t.currentLineBackground = QColor("#eee8d5");
    t.lineNumberForeground = QColor("#93a1a1");
    t.lineNumberBackground = QColor("#fdf6e3");

    t.keyword = QColor("#859900");
    t.string = QColor("#2aa198");
    t.comment = QColor("#93a1a1");
    t.number = QColor("#d33682");
    t.function = QColor("#268bd2");
    t.type = QColor("#b58900");
    t.variable = QColor("#657b83");
    t.operator_ = QColor("#859900");

    t.menuBackground = QColor("#eee8d5");
    t.menuForeground = QColor("#657b83");
    t.toolbarBackground = QColor("#eee8d5");
    t.statusBarBackground = QColor("#eee8d5");
    t.statusBarForeground = QColor("#586e75");
    t.tabActiveBackground = QColor("#fdf6e3");
    t.tabInactiveBackground = QColor("#eee8d5");
    t.tabForeground = QColor("#657b83");

    t.accentPrimary = QColor("#268bd2");
    t.accentSecondary = QColor("#2aa198");
    t.borderColor = QColor("#eee8d5");

    t.foldMarginBackground = QColor("#eee8d5");
    t.bookmarkMarginBackground = QColor("#fdf6e3");
    t.indentGuideColor = QColor("#d6cdb5");
    t.whitespaceColor = QColor("#d6cdb5");
    t.markHighlightColor = QColor("#e6db74");
    t.bracketMatchBackground = QColor("#b4deb4");
    t.bracketErrorBackground = QColor("#e8a0a0");

    t.errorForeground = QColor("#dc322f");
    t.warningForeground = QColor("#b58900");
    t.infoForeground = QColor("#268bd2");
    t.hintForeground = QColor("#93a1a1");

    return t;
}

Theme Theme::notepadpp()
{
    Theme t;
    t.name = "Notepad++";
    t.author = "Classic";
    t.isDark = false;

    t.background = QColor("#ffffff");
    t.foreground = QColor("#000000");
    t.selectionBackground = QColor("#0078d7");
    t.selectionForeground = QColor("#ffffff");
    t.currentLineBackground = QColor("#e8e8ff");
    t.lineNumberForeground = QColor("#808080");
    t.lineNumberBackground = QColor("#e4e4e4");

    t.keyword = QColor("#0000ff");
    t.string = QColor("#808080");
    t.comment = QColor("#008000");
    t.number = QColor("#ff8000");
    t.function = QColor("#000080");
    t.type = QColor("#0000ff");
    t.variable = QColor("#000000");
    t.operator_ = QColor("#000080");

    t.menuBackground = QColor("#f0f0f0");
    t.menuForeground = QColor("#000000");
    t.toolbarBackground = QColor("#f0f0f0");
    t.statusBarBackground = QColor("#f0f0f0");
    t.statusBarForeground = QColor("#000000");
    t.tabActiveBackground = QColor("#ffffff");
    t.tabInactiveBackground = QColor("#e8e8e8");
    t.tabForeground = QColor("#000000");

    t.accentPrimary = QColor("#0078d7");
    t.accentSecondary = QColor("#005a9e");
    t.borderColor = QColor("#c0c0c0");

    t.foldMarginBackground = QColor("#f0f0f0");
    t.bookmarkMarginBackground = QColor("#e4e4e4");
    t.indentGuideColor = QColor("#d0d0d0");
    t.whitespaceColor = QColor("#c0c0c0");
    t.markHighlightColor = QColor("#ffff00");
    t.bracketMatchBackground = QColor("#90ee90");
    t.bracketErrorBackground = QColor("#ff6666");

    t.errorForeground = QColor("#ff0000");
    t.warningForeground = QColor("#ff8000");
    t.infoForeground = QColor("#0000ff");
    t.hintForeground = QColor("#808080");

    return t;
}

// ThemeManager implementation
ThemeManager::ThemeManager()
{
    loadBuiltInThemes();
    m_currentTheme = Theme::olive();  // Set Olive as default theme
}

ThemeManager::~ThemeManager()
{
}

ThemeManager& ThemeManager::instance()
{
    static ThemeManager instance;
    return instance;
}

void ThemeManager::loadBuiltInThemes()
{
    m_themes["Olive"] = Theme::olive();
    m_themes["Default Dark"] = Theme::defaultDark();
    m_themes["Default Light"] = Theme::defaultLight();
    m_themes["Monokai"] = Theme::monokai();
    m_themes["Dracula"] = Theme::dracula();
    m_themes["Nord"] = Theme::nord();
    m_themes["Solarized Dark"] = Theme::solarizedDark();
    m_themes["Solarized Light"] = Theme::solarizedLight();
    m_themes["Notepad++"] = Theme::notepadpp();
}

void ThemeManager::loadThemes()
{
    QString dir = themesDirectory();
    QDir themesDir(dir);

    if (!themesDir.exists()) {
        return;
    }

    QStringList themeFiles = themesDir.entryList(QStringList() << "*.json", QDir::Files);
    for (const QString &fileName : themeFiles) {
        QString filePath = themesDir.absoluteFilePath(fileName);
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            Theme theme = Theme::fromJson(doc.object());
            m_themes[theme.name] = theme;
        }
    }
}

void ThemeManager::saveTheme(const Theme &theme)
{
    QString dir = themesDirectory();
    QDir().mkpath(dir);

    QString fileName = theme.name + ".json";
    QString filePath = dir + "/" + fileName;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(theme.toJson());
        file.write(doc.toJson(QJsonDocument::Indented));
        m_themes[theme.name] = theme;
    }
}

bool ThemeManager::deleteTheme(const QString &name)
{
    // Don't delete built-in themes
    QStringList builtIn = {"Default Dark", "Default Light", "Monokai", "Dracula", "Nord",
                            "Solarized Dark", "Solarized Light", "Notepad++"};
    if (builtIn.contains(name)) {
        return false;
    }

    QString dir = themesDirectory();
    QString filePath = dir + "/" + name + ".json";

    m_themes.remove(name);
    return QFile::remove(filePath);
}

QVector<Theme> ThemeManager::themes() const
{
    return m_themes.values().toVector();
}

Theme ThemeManager::theme(const QString &name) const
{
    return m_themes.value(name, Theme::defaultDark());
}

Theme ThemeManager::currentTheme() const
{
    return m_currentTheme;
}

void ThemeManager::applyTheme(const QString &name)
{
    if (m_themes.contains(name)) {
        applyTheme(m_themes[name]);
    }
}

void ThemeManager::applyTheme(const Theme &theme)
{
    m_currentTheme = theme;
    QString stylesheet = theme.toStyleSheet();
    qApp->setStyleSheet(stylesheet);
    emit themeChanged(m_currentTheme);
}

bool ThemeManager::importTheme(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    Theme theme = Theme::fromJson(doc.object());
    saveTheme(theme);
    return true;
}

bool ThemeManager::exportTheme(const QString &name, const QString &filePath)
{
    if (!m_themes.contains(name)) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonDocument doc(m_themes[name].toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

QString ThemeManager::themesDirectory() const
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dataDir + "/themes";
}

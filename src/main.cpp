#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QStandardPaths>
#include <QIcon>
#include "ui/mainwindow.h"
#include "ui/theme.h"
#include "utils/settings.h"
#include "core/session.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Olive Notepad");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("OliveNotepad");
    app.setOrganizationDomain("olive-notepad.org");
    // Set window icon - try olive icon first, fallback to legacy icons
    QIcon appIcon(":/icons/olive_notepad_icon.png");
    if (appIcon.isNull()) {
        appIcon = QIcon(":/icons/notepad-supreme.svg");
    }
    if (appIcon.isNull()) {
        appIcon = QIcon(":/icons/notepad_supreme_icon.png");
    }
    app.setWindowIcon(appIcon);

    // Ensure config directories exist
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(configPath);
    QDir().mkpath(dataPath);
    QDir().mkpath(dataPath + "/recovery");
    QDir().mkpath(dataPath + "/sessions");

    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("A native Linux text editor");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("files", "Files to open", "[files...]");

    QCommandLineOption newWindowOption("new-window", "Open in a new window");
    parser.addOption(newWindowOption);

    QCommandLineOption lineOption(QStringList() << "g" << "goto", "Go to line number", "line");
    parser.addOption(lineOption);

    parser.process(app);

    // Load settings
    Settings::instance().load();

    // Load and apply Olive theme
    ThemeManager::instance().loadThemes();
    ThemeManager::instance().applyTheme("Notepad++");

    // Apply theme stylesheet to application
    app.setStyleSheet(ThemeManager::instance().currentTheme().toStyleSheet());

    // Create main window
    MainWindow mainWindow;
    mainWindow.show();

    // Open files from command line
    const QStringList files = parser.positionalArguments();
    for (const QString &file : files) {
        mainWindow.openFile(file);
    }

    // Go to line if specified
    if (parser.isSet(lineOption)) {
        bool ok;
        int line = parser.value(lineOption).toInt(&ok);
        if (ok && line > 0) {
            mainWindow.goToLine(line);
        }
    }

    // Restore session if no files specified and setting enabled
    if (files.isEmpty() && Settings::instance().restoreSession()) {
        Session::instance().restore(&mainWindow);
    }

    int result = app.exec();

    // Save session on exit
    Session::instance().save(&mainWindow);
    Settings::instance().save();

    return result;
}

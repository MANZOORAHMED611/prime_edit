#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QStandardPaths>
#include <QIcon>
#include <QPixmap>
#include "ui/mainwindow.h"
#include "ui/editor.h"
#include "ui/recoverydialog.h"
#include "ui/theme.h"
#include "ui/tabwidget.h"
#include "core/document.h"
#include "utils/settings.h"
#include "core/session.h"
#include "core/pluginmanager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("PrimeEdit");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("PrimeEdit");
    app.setOrganizationDomain("primeedit.dev");
    // Set window icon — add at multiple sizes for proper WM display
    QIcon appIcon;
    QPixmap iconPixmap(":/icons/prime_edit_icon.png");
    if (!iconPixmap.isNull()) {
        // Add scaled versions for taskbar (48), titlebar (32), and small (16)
        appIcon.addPixmap(iconPixmap.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        appIcon.addPixmap(iconPixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        appIcon.addPixmap(iconPixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        appIcon.addPixmap(iconPixmap.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        appIcon.addPixmap(iconPixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        appIcon.addPixmap(iconPixmap.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    app.setWindowIcon(appIcon);

    // Set the desktop filename for proper icon association in GNOME/KDE
    app.setDesktopFileName("prime-edit");

    // Ensure config directories exist
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(configPath);
    QDir().mkpath(dataPath);
    QDir().mkpath(dataPath + "/recovery");
    QDir().mkpath(dataPath + "/sessions");

    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("The editor for structured knowledge work");
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

    // Load and apply theme from settings
    ThemeManager::instance().loadThemes();
    ThemeManager::instance().applyTheme(Settings::instance().theme());

    // Unsaved documents are restored silently by Session::restore()
    // via restoreUnsavedDocuments() — no dialog, Notepad++ behavior

    // Create main window
    MainWindow mainWindow;
    mainWindow.show();

    // Load plugins
    PluginManager::instance().loadPlugins();

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
        // Close the initial empty "Untitled 1" tab if session restored files
        if (mainWindow.tabWidget()->count() > 1) {
            Editor *first = qobject_cast<Editor*>(mainWindow.tabWidget()->widget(0));
            if (first && first->document() && first->document()->isUntitled() && first->toPlainText().isEmpty()) {
                mainWindow.closeFile(0);
            }
        }
    }

    int result = app.exec();

    return result;
}

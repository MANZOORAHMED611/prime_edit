#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QIcon>
#include <QMessageBox>
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
#include "core/singleinstance.h"

namespace {

// Window icon at several sizes for proper WM display.
void setApplicationIcon(QApplication &app)
{
    QIcon appIcon;
    QPixmap iconPixmap(":/icons/prime_edit_icon.png");
    if (!iconPixmap.isNull()) {
        // Taskbar (48), titlebar (32) and small (16), plus larger sizes.
        for (int size : {256, 128, 64, 48, 32, 16}) {
            appIcon.addPixmap(iconPixmap.scaled(size, size, Qt::KeepAspectRatio,
                                                Qt::SmoothTransformation));
        }
    }
    app.setWindowIcon(appIcon);
    // The desktop filename gives the icon association in GNOME/KDE.
    app.setDesktopFileName("prime-edit");
}

struct LaunchRequest {
    QStringList files;  // absolute, made so against this process's cwd
    int gotoLine = 0;   // 0 when no valid --goto was given
};

LaunchRequest parseArguments(QApplication &app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("The editor for structured knowledge work");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("files", "Files to open", "[files...]");
    // Forwarded like any other launch until one process supports several
    // windows (spec Q2).
    QCommandLineOption newWindowOption("new-window", "Open in a new window");
    parser.addOption(newWindowOption);
    QCommandLineOption lineOption(QStringList() << "g" << "goto", "Go to line number", "line");
    parser.addOption(lineOption);
    // --help and --version exit here, before the instance lock is touched.
    parser.process(app);

    LaunchRequest request;
    for (const QString &file : parser.positionalArguments()) {
        request.files.append(QFileInfo(file).absoluteFilePath());
    }
    bool ok = false;
    const int line = parser.value(lineOption).toInt(&ok);
    request.gotoLine = ok && line > 0 ? line : 0;
    return request;
}

void ensureDirectories()
{
    const QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    const QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(configPath);
    QDir().mkpath(dataPath);
    QDir().mkpath(dataPath + "/recovery");
    QDir().mkpath(dataPath + "/sessions");
}

// Another instance holds the profile: hand it the files. On no answer, report
// and exit 1; never start a second writer of the same cache.
int forwardToRunningInstance(const LaunchRequest &request)
{
    if (SingleInstance::forward(request.files, request.gotoLine)) {
        return 0;
    }
    const QString message = QObject::tr("PrimeEdit is running but not responding.");
    qCritical("%s", qPrintable(message));
    const QString platform = QGuiApplication::platformName();
    if (platform != QLatin1String("offscreen") && platform != QLatin1String("minimal")) {
        QMessageBox::critical(nullptr, QObject::tr("PrimeEdit"), message);
    }
    return 1;
}

void openRequested(MainWindow &window, const QStringList &files, int gotoLine)
{
    for (const QString &file : files) {
        window.openFile(file);
    }
    if (gotoLine > 0) {
        window.goToLine(gotoLine);
    }
}

// Drafts on every launch; saved files only under the preference; then the
// command line. The initial empty "Untitled" tab is closed as before.
void restoreAndOpen(MainWindow &window, const LaunchRequest &request)
{
    Session::instance().restoreDrafts(&window);
    if (Settings::instance().restoreSession()) {
        Session::instance().restoreFiles(&window);
    }
    openRequested(window, request.files, request.gotoLine);
    if (window.tabWidget()->count() > 1) {
        Editor *first = qobject_cast<Editor*>(window.tabWidget()->widget(0));
        if (first && first->document() && first->document()->isUntitled()
            && first->toPlainText().isEmpty()) {
            window.closeFile(0);
        }
    }
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("PrimeEdit");
    app.setApplicationVersion(QStringLiteral(PRIMEEDIT_VERSION));
    app.setOrganizationName("PrimeEdit");
    app.setOrganizationDomain("primeedit.dev");
    setApplicationIcon(app);

    const LaunchRequest request = parseArguments(app);
    ensureDirectories();

    // Spec 9.1 startup order: lock, socket, cache recovery, then settings
    // (whose corrupt-file copy is a write), then the window.
    SingleInstance instance;
    if (!instance.acquire()) {
        return forwardToRunningInstance(request);
    }
    if (!instance.listen()) {
        qWarning("Single-instance socket unavailable; later launches will not forward.");
    }
    Session::instance().recoverCacheDirectories();
    Settings::instance().load();
    ThemeManager::instance().loadThemes();
    ThemeManager::instance().applyTheme(Settings::instance().theme());

    MainWindow mainWindow;
    mainWindow.show();
    PluginManager::instance().loadPlugins();
    restoreAndOpen(mainWindow, request);

    QObject::connect(&instance, &SingleInstance::openRequested, &mainWindow,
                     [&mainWindow](const QStringList &files, int gotoLine) {
                         openRequested(mainWindow, files, gotoLine);
                         mainWindow.raise();
                         mainWindow.activateWindow();
                     }, Qt::QueuedConnection);

    return app.exec();
}

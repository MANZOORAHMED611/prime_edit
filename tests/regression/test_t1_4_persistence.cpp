// T1.4 regression tests (spec 2026-11-26-phase1-data-loss-fixes, D10): a
// truncated persistence file is copied aside, never overwritten with defaults,
// and nothing is written back until something actually changes.
#include "support.h"

#include "core/macrorecorder.h"
#include "core/session.h"
#include "ui/mainwindow.h"
#include "utils/settings.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>

namespace {

const QByteArray kTruncated("{\"editor\": {\"fontSize\": 14, \"tabWi");

QString configDir()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir;
}

QString dataDir()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir;
}

// Corrupt copies of `path`, named "<file>.corrupt-<yyyyMMdd-hhmmss>".
QStringList corruptCopies(const QString &path)
{
    const QFileInfo info(path);
    QDir dir(info.absolutePath());
    QStringList copies;
    for (const QString &name : dir.entryList({info.fileName() + ".corrupt-*"}, QDir::Files))
        copies.append(dir.filePath(name));
    return copies;
}

void removeCorruptCopies(const QString &path)
{
    for (const QString &copy : corruptCopies(path))
        QFile::remove(copy);
}

bool parsesAsObject(const QString &path)
{
    return QJsonDocument::fromJson(regression::readBytes(path)).isObject();
}

bool g_inForkedChild = false;

} // namespace

// LeakSanitizer hook (sanitizer/lsan_interface.h). The forked child exits with
// exit() while the parent's QApplication is never destroyed, so LSan would
// report Qt/glib state at exit and turn the exit code nonzero. Leak checking
// stays on for the parent process; only the child opts out.
extern "C" int __lsan_is_turned_off()
{
    return g_inForkedChild ? 1 : 0;
}

namespace {

// Runs `body` in a child that exits normally, so static destructors run (the
// MacroRecorder singleton saves from its destructor). Returns the exit code.
template<typename Body>
int runInChild(Body body)
{
    const pid_t pid = ::fork();
    if (pid == 0) {
        g_inForkedChild = true;
        body();
        std::exit(0);
    }
    int status = 0;
    ::waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

} // namespace

class TestT14Persistence : public QObject
{
    Q_OBJECT

private slots:
    void truncatedSettingsKeptAndNotOverwritten();
    void truncatedMacrosKeptAndNotOverwritten();
    void truncatedSessionKeptAndNotOverwritten();
    void truncatedNamedSessionKept();
};

void TestT14Persistence::truncatedSettingsKeptAndNotOverwritten()
{
    const QString path = configDir() + "/settings.json";
    removeCorruptCopies(path);
    QVERIFY(regression::writeBytes(path, kTruncated));
    Settings &settings = Settings::instance();

    settings.load();
    const QStringList copies = corruptCopies(path);
    QCOMPARE(copies.size(), 1);
    QCOMPARE(regression::readBytes(copies.first()), kTruncated);
    QCOMPARE(regression::readBytes(path), kTruncated);

    settings.save();
    QVERIFY2(regression::readBytes(path) == kTruncated, "defaults were saved over the file");

    const int size = settings.fontSize() + 3;
    settings.setFontSize(size);
    settings.save();
    QVERIFY(parsesAsObject(path));
    const QJsonObject root = QJsonDocument::fromJson(regression::readBytes(path)).object();
    QCOMPARE(root["editor"].toObject()["fontSize"].toInt(), size);
    settings.setFontSize(size - 3);
}

void TestT14Persistence::truncatedMacrosKeptAndNotOverwritten()
{
    const QString path = configDir() + "/macros.json";
    removeCorruptCopies(path);
    QVERIFY(regression::writeBytes(path, kTruncated));

    // Load, then exit: the destructor's save must not replace the file.
    QCOMPARE(runInChild([] { MacroRecorder::instance(); }), 0);
    QCOMPARE(regression::readBytes(path), kTruncated);
    const QStringList copies = corruptCopies(path);
    QVERIFY(!copies.isEmpty());
    QCOMPARE(regression::readBytes(copies.first()), kTruncated);

    // Saving a macro is a change, so the file is then written.
    QCOMPARE(runInChild([] {
                 MacroRecorder &macros = MacroRecorder::instance();
                 macros.startRecording();
                 macros.recordText("abc");
                 macros.stopRecording();
                 macros.saveMacro("typed");
             }),
             0);
    QVERIFY(parsesAsObject(path));
    QVERIFY(QJsonDocument::fromJson(regression::readBytes(path)).object().contains("typed"));
}

void TestT14Persistence::truncatedSessionKeptAndNotOverwritten()
{
    const QString path = dataDir() + "/session.json";
    removeCorruptCopies(path);
    QVERIFY(regression::writeBytes(path, kTruncated));
    MainWindow window;
    window.resize(800, 600);

    Session::instance().restore(&window);
    const QStringList copies = corruptCopies(path);
    QCOMPARE(copies.size(), 1);
    QCOMPARE(regression::readBytes(copies.first()), kTruncated);
    QCOMPARE(regression::readBytes(path), kTruncated);

    Session::instance().save(&window);
    QVERIFY2(regression::readBytes(path) == kTruncated, "unchanged session written over the file");

    window.resize(900, 640);
    Session::instance().save(&window);
    QVERIFY(parsesAsObject(path));
    QCOMPARE(QJsonDocument::fromJson(regression::readBytes(path)).object()["geometry"]
                 .toObject()["width"].toInt(), 900);
}

void TestT14Persistence::truncatedNamedSessionKept()
{
    const QString dir = dataDir() + "/sessions";
    QDir().mkpath(dir);
    const QString path = dir + "/broken.json";
    removeCorruptCopies(path);
    QVERIFY(regression::writeBytes(path, kTruncated));
    MainWindow window;

    Session::instance().loadSession("broken", &window);
    const QStringList copies = corruptCopies(path);
    QCOMPARE(copies.size(), 1);
    QCOMPARE(regression::readBytes(copies.first()), kTruncated);
    QCOMPARE(regression::readBytes(path), kTruncated);
    QVERIFY(Session::instance().savedSessions().contains("broken"));
}

PRIMEEDIT_REGRESSION_MAIN(TestT14Persistence)

#include "test_t1_4_persistence.moc"

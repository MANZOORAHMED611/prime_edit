// T1.10 regression tests (spec docs/superpowers/specs/2026-11-27-session-autosave.md):
// session.json is written periodically while the window runs, so the list of
// open files survives a crash; idle ticks never touch disk; a failed write is
// retried; the corrupt-file hold (D10) survives a real startup.
// Cases 1-5 fail on 1.0.1 (64552c5); 6-8 are guards that pass there too.
#include "support.h"

#include "core/session.h"
#include "ui/editor.h"
#include "ui/mainwindow.h"
#include "ui/tabwidget.h"
#include "core/document.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>

#include <sys/stat.h>
#include <unistd.h>

namespace {

constexpr int kIntervalMs = 200;
const QByteArray kTruncated("{\"files\": [{\"pa");
const QString kScratchMarker = QStringLiteral("/home-test_t1_10_session_autosave");

QString dataDir()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir;
}

QString sessionPath()
{
    return dataDir() + "/session.json";
}

// Paths listed in a session file; empty when it is absent or unparsable.
QStringList listedPaths(const QString &path)
{
    const QJsonObject root = QJsonDocument::fromJson(regression::readBytes(path)).object();
    QStringList paths;
    for (const QJsonValue &value : root["files"].toArray())
        paths.append(value.toObject()["path"].toString());
    return paths;
}

bool parsesAsObject(const QString &path)
{
    return QJsonDocument::fromJson(regression::readBytes(path)).isObject();
}

struct FileStamp {
    ino_t inode = 0;
    timespec mtime{};
    bool operator==(const FileStamp &o) const
    {
        return inode == o.inode && mtime.tv_sec == o.mtime.tv_sec
            && mtime.tv_nsec == o.mtime.tv_nsec;
    }
};

FileStamp stampOf(const QString &path)
{
    struct stat st{};
    FileStamp stamp;
    if (::stat(QFile::encodeName(path).constData(), &st) == 0) {
        stamp.inode = st.st_ino;
        stamp.mtime = st.st_mtim;
    }
    return stamp;
}

bool setMode(const QString &path, mode_t mode)
{
    return ::chmod(QFile::encodeName(path).constData(), mode) == 0;
}

// Restores 0700 on the data dir however the case ends.
struct ModeRestorer {
    QString path;
    ~ModeRestorer() { setMode(path, 0700); }
};

// Same order as main.cpp restoreAndOpen(): drop the initial empty Untitled tab
// once something else is open.
void closeEmptyUntitled(MainWindow &window)
{
    if (window.tabWidget()->count() <= 1)
        return;
    Editor *first = qobject_cast<Editor *>(window.tabWidget()->widget(0));
    if (first && first->document() && first->document()->isUntitled()
        && first->toPlainText().isEmpty()) {
        window.closeFile(0);
    }
}

// Process-level profile, as in test_t1_6_instance.cpp.
struct Profile {
    QString root;
    QString dataHome() const { return root + "/data"; }
    QString configHome() const { return root + "/config"; }
    QString appData() const { return dataHome() + "/PrimeEdit/PrimeEdit"; }
    QString session() const { return appData() + "/session.json"; }
};

Profile makeProfile(const QString &name)
{
    Profile profile{QDir::homePath() + "/t110-" + name};
    QDir(profile.root).removeRecursively();
    QDir().mkpath(profile.appData() + "/sessions/unsaved");
    QDir().mkpath(profile.configHome());
    return profile;
}

void stop(QProcess &process)
{
    if (process.state() != QProcess::NotRunning) {
        process.kill();
        process.waitForFinished(5000);
    }
}

// Stops a launched process however the case ends, including a failed QTRY.
struct StopOnExit {
    QProcess &process;
    ~StopOnExit() { stop(process); }
};

} // namespace

class TestT110SessionAutosave : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void openFileIsPersistedWithoutQuit();
    void closedFileIsDroppedWithoutQuit();
    void failedWriteIsRetried();
    void corruptHoldSurvivesRealStartup();
    void processRecordsFilesBeforeCrash();
    void noWriteBeforeEnabled();
    void unchangedStateDoesNotRewrite();
    void closeStopsAutosave();

private:
    QProcessEnvironment processEnvironment(const Profile &profile) const;

    QString m_bin;
    // ctest's runtime dir can exceed the 108-byte socket path limit (T1.6).
    QTemporaryDir m_runtime{QStringLiteral("/tmp/pe-rt-XXXXXX")};
};

void TestT110SessionAutosave::initTestCase()
{
    MainWindow::setSessionAutosaveIntervalForTest(kIntervalMs);
    m_bin = qEnvironmentVariable("PRIMEEDIT_BIN");
    QVERIFY2(QFileInfo(m_bin).isExecutable(), qPrintable(m_bin));
    QVERIFY2(!m_bin.startsWith("/usr/"), "refusing to run an installed binary");
    for (const char *name : {"HOME", "XDG_DATA_HOME", "XDG_CONFIG_HOME"}) {
        const QString value = qEnvironmentVariable(name);
        QVERIFY2(value.contains(kScratchMarker), qPrintable(QString("%1=%2").arg(name, value)));
    }
    QVERIFY(m_runtime.isValid());
    QVERIFY(setMode(m_runtime.path(), 0700));
    const QString realRuntime = QStringLiteral("/run/user/%1").arg(::getuid());
    QVERIFY2(!m_runtime.path().startsWith(realRuntime), qPrintable(m_runtime.path()));
    QVERIFY2(sessionPath().contains(kScratchMarker), qPrintable(sessionPath()));
}

void TestT110SessionAutosave::init()
{
    QVERIFY(setMode(dataDir(), 0700));
    QFile::remove(sessionPath());
    QVERIFY(!QFile::exists(sessionPath()));
}

QProcessEnvironment TestT110SessionAutosave::processEnvironment(const Profile &profile) const
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("XDG_DATA_HOME", profile.dataHome());
    env.insert("XDG_CONFIG_HOME", profile.configHome());
    env.insert("XDG_RUNTIME_DIR", m_runtime.path());
    env.insert("QT_QPA_PLATFORM", "offscreen");
    return env;
}

// Case 1: an opened file reaches session.json with no quit.
void TestT110SessionAutosave::openFileIsPersistedWithoutQuit()
{
    regression::TempDir dir;
    const QString file = dir.filePath("one.txt");
    QVERIFY(regression::writeBytes(file, "1\n"));
    MainWindow window;
    window.setSessionAutosaveEnabled(true);
    QVERIFY(window.openFile(file));
    QTRY_VERIFY_WITH_TIMEOUT(listedPaths(sessionPath()).contains(file), 1000);
}

// Case 2: a closed file leaves session.json with no quit.
void TestT110SessionAutosave::closedFileIsDroppedWithoutQuit()
{
    regression::TempDir dir;
    const QString first = dir.filePath("first.txt");
    const QString second = dir.filePath("second.txt");
    QVERIFY(regression::writeBytes(first, "1\n"));
    QVERIFY(regression::writeBytes(second, "2\n"));
    MainWindow window;
    window.setSessionAutosaveEnabled(true);
    QVERIFY(window.openFile(first));
    QVERIFY(window.openFile(second));
    QTRY_VERIFY_WITH_TIMEOUT(listedPaths(sessionPath()).contains(second), 1000);

    QVERIFY(window.closeFile(-1));  // the current tab: second.txt
    QTRY_COMPARE_WITH_TIMEOUT(listedPaths(sessionPath()), QStringList{first}, 1000);
}

// Case 3: a write that failed is retried on a later tick with no state change.
void TestT110SessionAutosave::failedWriteIsRetried()
{
    regression::TempDir dir;
    const QString file = dir.filePath("retry.txt");
    QVERIFY(regression::writeBytes(file, "r\n"));
    MainWindow window;
    QVERIFY(window.openFile(file));
    ModeRestorer restorer{dataDir()};
    QVERIFY(setMode(dataDir(), 0500));
    window.setSessionAutosaveEnabled(true);
    QTest::qWait(kIntervalMs * 3);
    QVERIFY2(!QFile::exists(sessionPath()), "written into a read-only directory");

    QVERIFY(setMode(dataDir(), 0700));
    QTRY_VERIFY_WITH_TIMEOUT(listedPaths(sessionPath()).contains(file), 2000);
}

// Case 4: startup itself (restore, file argument, Untitled closed, show) is not
// a user change, so the corrupt original survives; a bookmark releases the hold.
void TestT110SessionAutosave::corruptHoldSurvivesRealStartup()
{
    const QString cache = dataDir() + "/sessions/unsaved";
    QDir().mkpath(cache);
    const QJsonObject draft{{"text", "cached draft"}, {"originalFilePath", ""},
                            {"encoding", "UTF-8"}, {"title", "seeded"}};
    QVERIFY(regression::writeBytes(cache + "/t110-draft.json", QJsonDocument(draft).toJson()));
    QVERIFY(regression::writeBytes(sessionPath(), kTruncated));
    regression::TempDir dir;
    const QString arg = dir.filePath("argument.txt");
    QVERIFY(regression::writeBytes(arg, "a\nb\nc\n"));

    MainWindow window;
    Session::instance().restoreDrafts(&window);
    Session::instance().restoreFiles(&window);
    Editor *editor = window.openFile(arg);
    QVERIFY(editor);
    closeEmptyUntitled(window);
    window.show();
    window.setSessionAutosaveEnabled(true);
    QTest::qWait(kIntervalMs * 4);
    QVERIFY2(regression::readBytes(sessionPath()) == kTruncated,
             "startup was read as a user change and overwrote the corrupt file");

    editor->toggleBookmark(1);
    QTRY_VERIFY_WITH_TIMEOUT(parsesAsObject(sessionPath()), 2000);
    QVERIFY(listedPaths(sessionPath()).contains(arg));
    QFile::remove(cache + "/t110-draft.json");
}

// Case 5 (spec section 1 reproduction): a killed process's open files come back.
void TestT110SessionAutosave::processRecordsFilesBeforeCrash()
{
    const Profile profile = makeProfile("crash");
    QVERIFY2(profile.appData().contains(kScratchMarker), qPrintable(profile.appData()));
    regression::TempDir work;
    const QString file = work.filePath("kept.txt");
    QVERIFY(regression::writeBytes(file, "kept across a crash\n"));

    QProcess first;
    StopOnExit stopFirst{first};
    first.setProcessEnvironment(processEnvironment(profile));
    first.start(m_bin, {file});
    QVERIFY(first.waitForStarted(5000));
    QTRY_VERIFY_WITH_TIMEOUT(listedPaths(profile.session()).contains(file), 10000);
    first.kill();  // SIGKILL: no closeEvent, no quit-time save
    QVERIFY(first.waitForFinished(5000));
    const ino_t before = stampOf(profile.session()).inode;
    QVERIFY(before != 0);

    QProcess second;
    StopOnExit stopSecond{second};
    second.setProcessEnvironment(processEnvironment(profile));
    second.start(m_bin, {});
    QVERIFY(second.waitForStarted(5000));
    // Every QSaveFile commit is a new inode, and a fresh process always writes
    // its first tick, so a new inode listing the file proves it was reopened.
    QTRY_VERIFY_WITH_TIMEOUT(stampOf(profile.session()).inode != before
                                 && listedPaths(profile.session()).contains(file),
                             10000);
}

// Case 6 (guard): nothing is written before autosave is enabled.
void TestT110SessionAutosave::noWriteBeforeEnabled()
{
    regression::TempDir dir;
    const QString file = dir.filePath("never.txt");
    QVERIFY(regression::writeBytes(file, "n\n"));
    MainWindow window;
    QVERIFY(window.openFile(file));
    QTest::qWait(kIntervalMs * 3);
    QVERIFY(!QFile::exists(sessionPath()));
}

// Case 7 (guard): idle ticks do not rewrite an unchanged session.
void TestT110SessionAutosave::unchangedStateDoesNotRewrite()
{
    regression::TempDir dir;
    const QString file = dir.filePath("idle.txt");
    QVERIFY(regression::writeBytes(file, "i\n"));
    MainWindow window;
    QVERIFY(window.openFile(file));
    Session::instance().save(&window);
    QVERIFY(listedPaths(sessionPath()).contains(file));
    const FileStamp before = stampOf(sessionPath());

    window.setSessionAutosaveEnabled(true);
    QTest::qWait(kIntervalMs * 3 + kIntervalMs / 2);
    QVERIFY2(stampOf(sessionPath()) == before, "an unchanged session was rewritten");
}

// Case 8 (guard): closing the window stops the timer.
void TestT110SessionAutosave::closeStopsAutosave()
{
    MainWindow window;
    window.show();
    window.setSessionAutosaveEnabled(true);
    QVERIFY(window.close());
    QVERIFY(!window.sessionAutosaveActive());
}

PRIMEEDIT_REGRESSION_MAIN(TestT110SessionAutosave)

#include "test_t1_10_session_autosave.moc"

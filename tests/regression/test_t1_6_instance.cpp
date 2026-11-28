// T1.6 regression tests (spec 2026-11-26-phase1-data-loss-fixes, D4 single
// instance and startup). These launch only the build-tree $PRIMEEDIT_BIN, each
// case in its own scratch profile under the ctest HOME. initTestCase refuses to
// launch anything unless every path, the socket included, is in scratch.
#include "support.h"

#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QProcess>
#include <QTemporaryDir>

#include "core/singleinstance.h"

#include <atomic>
#include <thread>

#include <sys/stat.h>
#include <unistd.h>

namespace {

const QString kScratchMarker = QStringLiteral("/home-test_t1_6_instance");

// ctest's XDG_RUNTIME_DIR sits under the build tree, which can exceed the
// 108-byte limit of a Unix socket path. The suite uses its own short scratch
// runtime dir instead, created fresh and removed afterwards.
QString g_runtimeDir;

// One isolated profile: its own data and config dirs, shared scratch runtime dir.
struct Profile {
    QString root;
    QString dataHome() const { return root + "/data"; }
    QString configHome() const { return root + "/config"; }
    // QStandardPaths::AppDataLocation for organisation and app "PrimeEdit".
    QString appData() const { return dataHome() + "/PrimeEdit/PrimeEdit"; }
    QString cacheDir() const { return appData() + "/sessions/unsaved"; }
};

Profile makeProfile(const QString &name)
{
    Profile profile{QDir::homePath() + "/t16-" + name};
    QDir(profile.root).removeRecursively();
    QDir().mkpath(profile.appData() + "/sessions/unsaved");
    QDir().mkpath(profile.configHome());
    return profile;
}

// Spec 3 D4: $XDG_RUNTIME_DIR/prime-edit-<first 16 hex of SHA-1(AppDataLocation)>.sock
QString socketPathFor(const Profile &profile)
{
    const QByteArray hash = QCryptographicHash::hash(profile.appData().toUtf8(),
                                                     QCryptographicHash::Sha1).toHex();
    return g_runtimeDir + "/prime-edit-" + hash.left(16) + ".sock";
}

QProcessEnvironment environmentFor(const Profile &profile)
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("XDG_DATA_HOME", profile.dataHome());
    env.insert("XDG_CONFIG_HOME", profile.configHome());
    env.insert("XDG_RUNTIME_DIR", g_runtimeDir);
    env.insert("QT_QPA_PLATFORM", "offscreen");
    return env;
}

void writeDraft(const Profile &profile, const QString &name, const QString &text)
{
    QJsonObject obj{{"text", text}, {"originalFilePath", ""}, {"encoding", "UTF-8"},
                    {"title", "seeded"}};
    QVERIFY(regression::writeBytes(profile.cacheDir() + "/" + name, QJsonDocument(obj).toJson()));
}

// True once the running app's 30 s cache write has re-written `text` from an
// editor. Entries written from an editor carry "lineEnding"; a seeded entry
// that no editor holds is copied verbatim, without it.
bool draftCachedFromEditor(const Profile &profile, const QString &text)
{
    QDir dir(profile.cacheDir());
    for (const QString &name : dir.entryList({"*.json"}, QDir::Files)) {
        const QJsonObject obj =
            QJsonDocument::fromJson(regression::readBytes(dir.filePath(name))).object();
        if (obj["text"].toString() == text && obj.contains("lineEnding"))
            return true;
    }
    return false;
}

QByteArray snapshot(const QString &dir)
{
    QByteArray all;
    for (const QString &name : QDir(dir).entryList(QDir::Files, QDir::Name))
        all += name.toUtf8() + '\n' + regression::readBytes(dir + "/" + name);
    return all;
}

void stop(QProcess &process)
{
    if (process.state() != QProcess::NotRunning) {
        process.kill();
        process.waitForFinished(5000);
    }
}

// Plays the running instance: holds the profile's lock and listens on its
// socket, so the launched binary must forward to it.
struct FakeHolder {
    explicit FakeHolder(const Profile &profile) : lock(profile.appData() + "/instance.lock")
    {
        locked = lock.tryLock(0);
        QLocalServer::removeServer(socketPathFor(profile));
        listening = server.listen(socketPathFor(profile));
    }
    QLockFile lock;
    QLocalServer server;
    bool locked = false;
    bool listening = false;
};

} // namespace

class TestT16Instance : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void secondLaunchForwardsAbsolutePathAndExitsZero();
    void holderThatNeverAcksMakesSecondLaunchExitOne();
    void launchWithFileArgumentRestoresDrafts();
    void corruptSessionJsonStillRestoresDrafts();
    void realHolderAcceptsForwardedLaunch();
    void forwardAckedDuringStartupIsDelivered();

private:
    QString m_bin;
    QTemporaryDir m_runtime{QStringLiteral("/tmp/pe-rt-XXXXXX")};
};

void TestT16Instance::initTestCase()
{
    m_bin = qEnvironmentVariable("PRIMEEDIT_BIN");
    QVERIFY2(QFileInfo(m_bin).isExecutable(), qPrintable(m_bin));
    QVERIFY2(!m_bin.startsWith("/usr/"), "refusing to run an installed binary");
    for (const char *name : {"HOME", "XDG_DATA_HOME", "XDG_CONFIG_HOME", "XDG_RUNTIME_DIR"}) {
        const QString value = qEnvironmentVariable(name);
        QVERIFY2(value.contains(kScratchMarker), qPrintable(QString("%1=%2").arg(name, value)));
    }
    QVERIFY(m_runtime.isValid());
    g_runtimeDir = m_runtime.path();
    QVERIFY(::chmod(QFile::encodeName(g_runtimeDir).constData(), 0700) == 0);
    const QString realRuntime = QStringLiteral("/run/user/%1").arg(::getuid());
    QVERIFY2(!g_runtimeDir.startsWith(realRuntime), qPrintable(g_runtimeDir));
    const Profile probe{QDir::homePath() + "/t16-probe"};
    QVERIFY2(probe.appData().contains(kScratchMarker), qPrintable(probe.appData()));
    // The socket every launch will use must resolve inside the scratch dir.
    const QString socket = socketPathFor(probe);
    QVERIFY2(socket.startsWith(g_runtimeDir + "/"), qPrintable(socket));
    QVERIFY2(socket.toUtf8().size() < 104, qPrintable(socket));
}

void TestT16Instance::secondLaunchForwardsAbsolutePathAndExitsZero()
{
    const Profile profile = makeProfile("forward");
    writeDraft(profile, "holder.json", "the holder's draft");
    const QByteArray draftsBefore = snapshot(profile.cacheDir());
    regression::TempDir work;
    QVERIFY(regression::writeBytes(work.filePath("relative.txt"), "x\n"));
    FakeHolder holder(profile);
    QVERIFY(holder.locked && holder.listening);

    QProcess second;
    second.setProcessEnvironment(environmentFor(profile));
    second.setWorkingDirectory(work.path());
    second.start(m_bin, {"relative.txt", "--goto", "7"});
    QVERIFY(second.waitForStarted(5000));
    QTRY_VERIFY_WITH_TIMEOUT(holder.server.hasPendingConnections(), 10000);
    QLocalSocket *client = holder.server.nextPendingConnection();
    QTRY_VERIFY_WITH_TIMEOUT(client->canReadLine(), 5000);
    const QJsonObject message = QJsonDocument::fromJson(client->readLine()).object();
    client->write("{\"ack\":true}\n");
    client->flush();

    QVERIFY(second.waitForFinished(10000));
    stop(second);
    QCOMPARE(second.exitCode(), 0);
    QCOMPARE(message["v"].toInt(), 1);
    QCOMPARE(message["files"].toArray(), QJsonArray{work.filePath("relative.txt")});
    QCOMPARE(message["goto"].toInt(), 7);
    QCOMPARE(snapshot(profile.cacheDir()), draftsBefore);
}

void TestT16Instance::holderThatNeverAcksMakesSecondLaunchExitOne()
{
    const Profile profile = makeProfile("noack");
    FakeHolder holder(profile);
    QVERIFY(holder.locked && holder.listening);

    QProcess second;
    second.setProcessEnvironment(environmentFor(profile));
    QElapsedTimer timer;
    timer.start();
    second.start(m_bin, {});
    QVERIFY(second.waitForStarted(5000));
    // Keep the event loop turning so the connection is accepted, never answered.
    QTRY_VERIFY_WITH_TIMEOUT(second.state() == QProcess::NotRunning, 15000);
    stop(second);
    QCOMPARE(second.exitStatus(), QProcess::NormalExit);
    QCOMPARE(second.exitCode(), 1);
    QVERIFY2(timer.elapsed() < 12000, "the second launch must give up after the ack timeout");
}

void TestT16Instance::launchWithFileArgumentRestoresDrafts()
{
    const Profile profile = makeProfile("fileargs");
    writeDraft(profile, "draft.json", "draft kept across a launch with files");
    regression::TempDir work;
    QVERIFY(regression::writeBytes(work.filePath("arg.txt"), "argument file\n"));

    QProcess app;
    app.setProcessEnvironment(environmentFor(profile));
    app.start(m_bin, {work.filePath("arg.txt")});
    QVERIFY(app.waitForStarted(5000));
    // The periodic writer runs every 30 s; give it one cycle plus startup.
    QTRY_VERIFY_WITH_TIMEOUT(
        draftCachedFromEditor(profile, "draft kept across a launch with files"), 45000);
    stop(app);
}

void TestT16Instance::corruptSessionJsonStillRestoresDrafts()
{
    const Profile profile = makeProfile("corruptsession");
    writeDraft(profile, "draft.json", "draft kept despite a torn session.json");
    QVERIFY(regression::writeBytes(profile.appData() + "/session.json", "{\"files\": [{\"pa"));

    QProcess app;
    app.setProcessEnvironment(environmentFor(profile));
    app.start(m_bin, {});
    QVERIFY(app.waitForStarted(5000));
    QTRY_VERIFY_WITH_TIMEOUT(
        draftCachedFromEditor(profile, "draft kept despite a torn session.json"), 45000);
    stop(app);
}

// Both sides real: the first launch holds the profile and listens; the second
// forwards to it and exits 0 instead of becoming a second writer.
void TestT16Instance::realHolderAcceptsForwardedLaunch()
{
    const Profile profile = makeProfile("realholder");
    regression::TempDir work;
    QVERIFY(regression::writeBytes(work.filePath("forwarded.txt"), "x\n"));

    QProcess holder;
    holder.setProcessEnvironment(environmentFor(profile));
    holder.start(m_bin, {});
    QVERIFY(holder.waitForStarted(5000));
    QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(socketPathFor(profile)), 15000);

    QProcess second;
    second.setProcessEnvironment(environmentFor(profile));
    second.start(m_bin, {work.filePath("forwarded.txt")});
    QVERIFY(second.waitForStarted(5000));
    const bool finished = second.waitForFinished(10000);
    stop(second);
    const bool holderAlive = holder.state() == QProcess::Running;
    stop(holder);
    QVERIFY2(finished, "the second launch kept running as a second instance");
    QCOMPARE(second.exitCode(), 0);
    QVERIFY(holderAlive);
}

// T1.9 m4: main.cpp listens before restore but connects openRequested only
// after it, and a dialog shown during restore runs the event loop. A forward
// acked in that window must still reach the receiver once it connects.
void TestT16Instance::forwardAckedDuringStartupIsDelivered()
{
    qputenv("XDG_RUNTIME_DIR", g_runtimeDir.toUtf8());
    QVERIFY2(SingleInstance::serverName().startsWith(g_runtimeDir + "/"),
             qPrintable(SingleInstance::serverName()));
    QVERIFY2(SingleInstance::lockPath().contains(kScratchMarker),
             qPrintable(SingleInstance::lockPath()));
    SingleInstance holder;
    QVERIFY(holder.acquire());
    QVERIFY(holder.listen());

    const QString file = QStringLiteral("/tmp/forwarded-during-restore.txt");
    std::atomic<bool> done{false};
    bool acked = false;
    std::thread client([&] {
        acked = SingleInstance::forward({file}, 3);
        done = true;
    });
    QTRY_VERIFY_WITH_TIMEOUT(done.load(), 10000);
    client.join();
    QVERIFY(acked);

    QStringList received;
    int line = 0;
    QObject::connect(&holder, &SingleInstance::openRequested, &holder,
                     [&](const QStringList &files, const OpenOptions &options) {
                         received = files;
                         line = options.line;
                     });
    QTRY_COMPARE_WITH_TIMEOUT(received, QStringList{file}, 3000);
    QCOMPARE(line, 3);
}

PRIMEEDIT_REGRESSION_MAIN(TestT16Instance)

#include "test_t1_6_instance.moc"

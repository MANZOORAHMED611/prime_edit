// P1.12 regression tests (spec docs/superpowers/specs/2026-11-28-atomic-short-write.md,
// section 5 cases 1-4): an atomic save whose write(2) fails - short, or writing
// nothing at all - reports failure at every payload size and leaves the
// destination byte-identical. Qt's QSaveFile buffers up to 16 KiB, so without
// QIODevice::Unbuffered a failed write inside that buffer was reported as success.
#include "support.h"

#include "core/document.h"
#include "core/session.h"
#include "ui/editor.h"
#include "ui/mainwindow.h"
#include "utils/fileutils.h"

#include <QDir>
#include <QJsonDocument>
#include <QMap>
#include <QStandardPaths>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

const QString kScratchMarker = QStringLiteral("/home-test_t1_12_short_write");
constexpr rlim_t kLimit = 4096;
constexpr int kChildSetupFailed = 0xFF;

// Every size the spec sweeps; 4097-16384 sat inside Qt's write buffer.
const QList<qsizetype> kSweepSizes{4097, 5000, 8192, 15000, 16383, 16384, 16385, 20000, 65536};
const QList<qsizetype> kUnderLimitSizes{1, 2048, 4000};

QByteArray originalBytes()
{
    QByteArray bytes(1023, 'o');
    bytes.append('\n');
    return bytes;
}

// Called in a forked child after all setup: a write past `limit` bytes fails
// with EFBIG instead of raising SIGXFSZ, the way ENOSPC fails a write.
void limitFileSize(rlim_t limit)
{
    ::signal(SIGXFSZ, SIG_IGN);
    struct rlimit rl {limit, limit};
    ::setrlimit(RLIMIT_FSIZE, &rl);
}

// The child's exit code, or -1 if it did not exit normally or its setup failed.
int waitForChild(pid_t pid)
{
    int status = 0;
    ::waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) == kChildSetupFailed)
        return -1;
    return WEXITSTATUS(status);
}

QByteArray readAllFrom(int fd)
{
    QByteArray bytes;
    char buf[512];
    ssize_t n = 0;
    while ((n = ::read(fd, buf, sizeof buf)) > 0)
        bytes.append(buf, n);
    return bytes;
}

struct DocReport {
    bool ran = false;
    SaveResult result = SaveResult::Ok;
    bool modified = false;
    QString lastError;
};

// Loads `path`, sets `size` bytes of new ASCII text and saves under `limit`.
// The exit code packs bits 0-3 SaveResult and bit 4 isModified(); lastError()
// comes back through a pipe.
DocReport saveDocumentUnderLimit(const QString &path, qsizetype size, rlim_t limit)
{
    int fds[2];
    if (::pipe(fds) != 0)
        return {};
    const pid_t pid = ::fork();
    if (pid == 0) {
        ::close(fds[0]);
        Document doc;
        if (!doc.load(path))
            ::_exit(kChildSetupFailed);
        doc.setText(QString(size, QLatin1Char('n')));
        limitFileSize(limit);
        const SaveResult result = doc.save();
        const QByteArray error = doc.lastError().toUtf8();
        const bool sent = ::write(fds[1], error.constData(), error.size()) == error.size();
        ::_exit(sent ? static_cast<int>(result) | (doc.isModified() ? 0x10 : 0)
                     : kChildSetupFailed);
    }
    ::close(fds[1]);
    const QByteArray error = readAllFrom(fds[0]);
    ::close(fds[0]);
    const int bits = waitForChild(pid);
    if (bits < 0)
        return {};
    return {true, static_cast<SaveResult>(bits & 0x0F), bool(bits & 0x10),
            QString::fromUtf8(error)};
}

// Exit code 1 when writeFileAtomic returned true, 0 when it returned false.
int writeFileAtomicUnderLimit(const QString &path, qsizetype size, rlim_t limit)
{
    const QByteArray bytes(size, 'w');
    const pid_t pid = ::fork();
    if (pid == 0) {
        limitFileSize(limit);
        ::_exit(FileUtils::writeFileAtomic(path, bytes) ? 1 : 0);
    }
    return waitForChild(pid);
}

// Returns what went wrong with one failed-save run, or an empty string.
QString failedDocumentSaveProblem(const QString &path, qsizetype size, rlim_t limit)
{
    if (!regression::writeBytes(path, originalBytes()))
        return QStringLiteral("fixture not written");
    const DocReport r = saveDocumentUnderLimit(path, size, limit);
    QStringList problems;
    if (!r.ran)
        return QStringLiteral("child did not run the save");
    if (r.result != SaveResult::IoError)
        problems << QStringLiteral("result %1").arg(int(r.result));
    if (!r.modified)
        problems << QStringLiteral("not modified");
    if (regression::readBytes(path) != originalBytes())
        problems << QStringLiteral("disk %1 bytes").arg(regression::readBytes(path).size());
    if (r.lastError != QString::fromLocal8Bit(std::strerror(EFBIG)))
        problems << QStringLiteral("lastError [%1]").arg(r.lastError);
    return problems.join(QStringLiteral(", "));
}

QString failedWriteFileAtomicProblem(const QString &path, qsizetype size, rlim_t limit)
{
    if (!regression::writeBytes(path, originalBytes()))
        return QStringLiteral("fixture not written");
    const int returned = writeFileAtomicUnderLimit(path, size, limit);
    QStringList problems;
    if (returned < 0)
        return QStringLiteral("child did not run");
    if (returned != 0)
        problems << QStringLiteral("returned true");
    if (regression::readBytes(path) != originalBytes())
        problems << QStringLiteral("disk %1 bytes").arg(regression::readBytes(path).size());
    return problems.join(QStringLiteral(", "));
}

using ProblemCheck = QString (*)(const QString &, qsizetype, rlim_t);

// Runs every size and lists every failing one, so a regression shows its range.
QStringList sweepProblems(ProblemCheck check, const QString &path, const QList<qsizetype> &sizes,
                          rlim_t limit)
{
    QStringList failures;
    for (const qsizetype size : sizes) {
        const QString problem = check(path, size, limit);
        if (!problem.isEmpty())
            failures << QStringLiteral("size %1 limit %2: %3").arg(size).arg(limit).arg(problem);
    }
    return failures;
}

QString cacheDir()
{
    return FileUtils::unsavedCacheBase();
}

void resetCache()
{
    for (const QString &suffix : {"", ".new", ".old", "_tmp", ".corrupt"})
        QDir(cacheDir() + suffix).removeRecursively();
    QDir().mkpath(cacheDir());
}

// Entry name -> exact bytes of every file in the draft cache.
QMap<QString, QByteArray> cacheSnapshot()
{
    QMap<QString, QByteArray> snapshot;
    for (const QString &name : QDir(cacheDir()).entryList(QDir::Files))
        snapshot.insert(name, regression::readBytes(cacheDir() + "/" + name));
    return snapshot;
}

// Exit code 1 when saveUnsavedDocuments returned true, 0 when it returned false.
int saveDraftsUnderLimit(MainWindow &window, rlim_t limit)
{
    const pid_t pid = ::fork();
    if (pid == 0) {
        limitFileSize(limit);
        ::_exit(Session::instance().saveUnsavedDocuments(&window) ? 1 : 0);
    }
    return waitForChild(pid);
}

} // namespace

class TestT112ShortWrite : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void documentSaveFailsAtEverySize();
    void documentSaveFailsWhenNothingCanBeWritten();
    void writeFileAtomicFailsAtEverySize();
    void writeFileAtomicFailsWhenNothingCanBeWritten();
    void draftCacheKeepsPreviousSet();
    void underLimitSavesStillSucceed();
};

void TestT112ShortWrite::initTestCase()
{
    for (const char *name : {"HOME", "XDG_DATA_HOME", "XDG_CONFIG_HOME"}) {
        const QString value = qEnvironmentVariable(name);
        QVERIFY2(value.contains(kScratchMarker), qPrintable(QString("%1=%2").arg(name, value)));
    }
    QVERIFY2(cacheDir().contains(kScratchMarker), qPrintable(cacheDir()));
}

// Case 1: under RLIMIT_FSIZE 4096 every size fails, keeps the original and
// reports the errno message, not Qt's "Unknown error".
void TestT112ShortWrite::documentSaveFailsAtEverySize()
{
    regression::TempDir dir;
    const QString path = dir.filePath("doc.txt");
    const QStringList failures =
        sweepProblems(failedDocumentSaveProblem, path, kSweepSizes, kLimit);
    QVERIFY2(failures.isEmpty(), qPrintable(failures.join(QStringLiteral("\n"))));
    QCOMPARE(QDir(dir.path()).entryList(QDir::Files), QStringList{"doc.txt"});
}

// Case 1, limit 0: the write stores nothing at all (spec R1-3).
void TestT112ShortWrite::documentSaveFailsWhenNothingCanBeWritten()
{
    regression::TempDir dir;
    const QString path = dir.filePath("doc.txt");
    const QString problem = failedDocumentSaveProblem(path, 8192, 0);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
    QCOMPARE(QDir(dir.path()).entryList(QDir::Files), QStringList{"doc.txt"});
}

// Case 2: the helper behind settings, sessions, macros and the draft cache.
void TestT112ShortWrite::writeFileAtomicFailsAtEverySize()
{
    regression::TempDir dir;
    const QString path = dir.filePath("settings.json");
    const QStringList failures =
        sweepProblems(failedWriteFileAtomicProblem, path, kSweepSizes, kLimit);
    QVERIFY2(failures.isEmpty(), qPrintable(failures.join(QStringLiteral("\n"))));
    QCOMPARE(QDir(dir.path()).entryList(QDir::Files), QStringList{"settings.json"});
}

void TestT112ShortWrite::writeFileAtomicFailsWhenNothingCanBeWritten()
{
    regression::TempDir dir;
    const QString path = dir.filePath("settings.json");
    const QString problem = failedWriteFileAtomicProblem(path, 8192, 0);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
    QCOMPARE(QDir(dir.path()).entryList(QDir::Files), QStringList{"settings.json"});
}

// Case 3: a draft whose serialized size sits inside Qt's buffer cannot be
// stored; the previous good set must survive with the same names and bytes.
void TestT112ShortWrite::draftCacheKeepsPreviousSet()
{
    resetCache();
    MainWindow window;
    Editor *editor = window.currentEditor();
    QVERIFY(editor);
    editor->setPlainText(QString(8000, QLatin1Char('a')));
    QVERIFY(Session::instance().saveUnsavedDocuments(&window));
    const QMap<QString, QByteArray> previous = cacheSnapshot();
    QCOMPARE(previous.size(), 1);
    const qsizetype serialized = previous.first().size();
    QVERIFY2(serialized >= 4097 && serialized <= 16384, qPrintable(QString::number(serialized)));

    editor->setPlainText(QString(8000, QLatin1Char('b')));
    QCOMPARE(saveDraftsUnderLimit(window, kLimit), 0);

    QCOMPARE(cacheSnapshot(), previous);
    for (const QByteArray &bytes : previous) {
        QJsonParseError error{};
        QJsonDocument::fromJson(bytes, &error);
        QCOMPARE(error.error, QJsonParseError::NoError);
    }
    QVERIFY(!QDir(cacheDir() + ".new").exists());
}

// Case 4: writes that fit under the limit still save with the new bytes.
void TestT112ShortWrite::underLimitSavesStillSucceed()
{
    regression::TempDir dir;
    const QString docPath = dir.filePath("doc.txt");
    const QString rawPath = dir.filePath("raw.json");
    for (const qsizetype size : kUnderLimitSizes) {
        QVERIFY(regression::writeBytes(docPath, originalBytes()));
        const DocReport r = saveDocumentUnderLimit(docPath, size, kLimit);
        QVERIFY2(r.ran, "child did not run the save");
        QCOMPARE(r.result, SaveResult::Ok);
        QVERIFY(!r.modified);
        QCOMPARE(regression::readBytes(docPath), QByteArray(size, 'n'));

        QVERIFY(regression::writeBytes(rawPath, originalBytes()));
        QCOMPARE(writeFileAtomicUnderLimit(rawPath, size, kLimit), 1);
        QCOMPARE(regression::readBytes(rawPath), QByteArray(size, 'w'));
    }
}

PRIMEEDIT_REGRESSION_MAIN(TestT112ShortWrite)
#include "test_t1_12_short_write.moc"

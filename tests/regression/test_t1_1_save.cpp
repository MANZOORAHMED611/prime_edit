// T1.1 regression tests (spec 2026-11-26-phase1-data-loss-fixes, D7, D3 core,
// D12 core): Document::save is atomic, checked, and refuses what it must not write.
#include "support.h"

#include "core/document.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>

#include <csignal>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

QByteArray sha1Of(const QString &path)
{
    return QCryptographicHash::hash(regression::readBytes(path), QCryptographicHash::Sha1);
}

struct stat statOf(const QString &path)
{
    struct stat st {};
    ::stat(QFile::encodeName(path).constData(), &st);
    return st;
}

// Restores a directory's mode on scope exit so QTemporaryDir can delete it.
class ModeGuard
{
public:
    ModeGuard(const QString &path, mode_t mode) : m_path(QFile::encodeName(path))
    {
        ::chmod(m_path.constData(), mode);
    }
    ~ModeGuard() { ::chmod(m_path.constData(), 0700); }

private:
    QByteArray m_path;
};

// Runs a save in a child process whose file-size limit is far below the new
// content, so the write fails the way ENOSPC does. Returns the child's report:
// bits 0-3 SaveResult, bit 4 isModified() after the save, bit 5
// directWriteAvailable(); -1 if the child did not exit normally.
int saveUnderFileSizeLimit(const QString &path, const QString &newText)
{
    const pid_t pid = ::fork();
    if (pid == 0) {
        Document doc;
        if (!doc.load(path))
            ::_exit(100);
        doc.setText(newText);
        ::signal(SIGXFSZ, SIG_IGN);
        struct rlimit limit {4096, 4096};
        ::setrlimit(RLIMIT_FSIZE, &limit);
        const SaveResult result = doc.save();
        const int report = static_cast<int>(result) | (doc.isModified() ? 0x10 : 0)
            | (doc.directWriteAvailable() ? 0x20 : 0);
        ::_exit(report);
    }
    int status = 0;
    ::waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

QByteArray manyLines(qint64 minBytes)
{
    const QByteArray line("The quick brown fox jumps over the lazy dog 0123456789\n");
    QByteArray data;
    data.reserve(minBytes + line.size());
    while (data.size() <= minBytes)
        data += line;
    return data;
}

} // namespace

class TestT11Save : public QObject
{
    Q_OBJECT

private slots:
    void enospcLeavesOriginalIntact();
    void readOnlyFileGivesIoError();
    void readOnlyDirectoryOffersDirectWrite();
    void hardLinkIsKept();
    void symlinkAndModeKeptWithAtomicReplace();
    void readOnlyDocumentRefusesSave();
    void partialViewOver50MbRefusesSaveAndConversion();
    void minifiedOver20MbRefusesSave();
};

void TestT11Save::enospcLeavesOriginalIntact()
{
    regression::TempDir dir;
    const QString path = dir.filePath("enospc.txt");
    const QByteArray original("original content\n");
    QVERIFY(regression::writeBytes(path, original));

    const int report = saveUnderFileSizeLimit(path, QString(64 * 1024, QLatin1Char('x')));
    QVERIFY2(report >= 0 && report != 100, "child did not run the save");
    QCOMPARE(static_cast<SaveResult>(report & 0x0F), SaveResult::IoError);
    QVERIFY2(report & 0x10, "document must stay modified after a failed save");
    QVERIFY2(!(report & 0x20), "a direct write after ENOSPC would truncate the original");
    QCOMPARE(regression::readBytes(path), original);
    QCOMPARE(QDir(dir.path()).entryList(QDir::Files), QStringList{"enospc.txt"});
}

void TestT11Save::readOnlyFileGivesIoError()
{
    regression::TempDir dir;
    const QString path = dir.filePath("ro.txt");
    const QByteArray original("keep me\n");
    QVERIFY(regression::writeBytes(path, original));
    Document doc;
    QVERIFY(doc.load(path));
    QVERIFY(::chmod(QFile::encodeName(path).constData(), 0444) == 0);
    doc.setText("changed\n");

    QCOMPARE(doc.save(), SaveResult::IoError);
    QVERIFY2(doc.lastError().contains("read-only", Qt::CaseInsensitive), qPrintable(doc.lastError()));
    QVERIFY(!doc.directWriteAvailable());
    QVERIFY(doc.isModified());
    QCOMPARE(doc.save(WriteMode::AllowDirect), SaveResult::IoError);
    QCOMPARE(regression::readBytes(path), original);
}

void TestT11Save::readOnlyDirectoryOffersDirectWrite()
{
    regression::TempDir dir;
    const QString sub = dir.filePath("locked");
    QVERIFY(QDir().mkpath(sub));
    const QString path = sub + "/file.txt";
    const QByteArray original("before\n");
    QVERIFY(regression::writeBytes(path, original));
    ModeGuard guard(sub, 0555);
    Document doc;
    QVERIFY(doc.load(path));
    doc.setText("after\n");

    QCOMPARE(doc.save(), SaveResult::IoError);
    QVERIFY(doc.directWriteAvailable());
    QVERIFY(doc.isModified());
    QCOMPARE(regression::readBytes(path), original);

    QCOMPARE(doc.save(WriteMode::AllowDirect), SaveResult::Ok);
    QCOMPARE(regression::readBytes(path), QByteArray("after\n"));
    QVERIFY(!doc.isModified());
}

void TestT11Save::hardLinkIsKept()
{
    regression::TempDir dir;
    const QString path = dir.filePath("a.txt");
    const QString link = dir.filePath("b.txt");
    QVERIFY(regression::writeBytes(path, "one\n"));
    QVERIFY(::link(QFile::encodeName(path).constData(), QFile::encodeName(link).constData()) == 0);
    Document doc;
    QVERIFY(doc.load(path));
    doc.setText("two\n");

    QCOMPARE(doc.save(), SaveResult::Ok);
    QVERIFY(doc.lastSaveWasDirect());
    QCOMPARE(statOf(path).st_ino, statOf(link).st_ino);
    QCOMPARE(statOf(path).st_nlink, nlink_t(2));
    QCOMPARE(regression::readBytes(link), QByteArray("two\n"));
}

void TestT11Save::symlinkAndModeKeptWithAtomicReplace()
{
    regression::TempDir dir;
    const QString target = dir.filePath("target.txt");
    const QString link = dir.filePath("link.txt");
    QVERIFY(regression::writeBytes(target, "old\n"));
    QVERIFY(::chmod(QFile::encodeName(target).constData(), 0640) == 0);
    QVERIFY(QFile::link(target, link));
    const ino_t inodeBefore = statOf(target).st_ino;
    Document doc;
    QVERIFY(doc.load(link));
    doc.setText("new\n");

    QCOMPARE(doc.save(), SaveResult::Ok);
    QVERIFY(!doc.lastSaveWasDirect());
    QVERIFY(QFileInfo(link).isSymLink());
    QCOMPARE(regression::readBytes(target), QByteArray("new\n"));
    QCOMPARE(statOf(target).st_mode & 07777, mode_t(0640));
    // An atomic replace renames a new file over the target: the inode changes.
    QVERIFY(statOf(target).st_ino != inodeBefore);
    QCOMPARE(QDir(dir.path()).entryList(QDir::Files | QDir::System).size(), 2);
}

void TestT11Save::readOnlyDocumentRefusesSave()
{
    regression::TempDir dir;
    const QString path = dir.filePath("doc.txt");
    QVERIFY(regression::writeBytes(path, "text\n"));
    Document doc;
    QVERIFY(doc.load(path));
    doc.setText("edited\n");
    doc.setReadOnly(true);

    QCOMPARE(doc.save(), SaveResult::ReadOnly);
    QCOMPARE(doc.saveAs(dir.filePath("other.txt")), SaveResult::ReadOnly);
    QVERIFY(doc.isModified());
    QCOMPARE(regression::readBytes(path), QByteArray("text\n"));
    QVERIFY(!QFile::exists(dir.filePath("other.txt")));
}

void TestT11Save::partialViewOver50MbRefusesSaveAndConversion()
{
    regression::TempDir dir;
    const QString path = dir.filePath("big.txt");
    QVERIFY(regression::writeBytes(path, manyLines(51LL * 1024 * 1024)));
    const QByteArray before = sha1Of(path);
    Document doc;
    QVERIFY(doc.load(path));
    QVERIFY(doc.isPartialView());

    doc.setEncoding("UTF-16LE");
    doc.setLineEnding(Document::Windows);
    QCOMPARE(doc.encoding(), QString("UTF-8"));
    QVERIFY(!doc.isModified());
    QCOMPARE(doc.save(), SaveResult::PartialView);
    QCOMPARE(doc.saveAs(dir.filePath("copy.txt")), SaveResult::PartialView);
    QCOMPARE(sha1Of(path), before);
    QVERIFY(!QFile::exists(dir.filePath("copy.txt")));
}

void TestT11Save::minifiedOver20MbRefusesSave()
{
    regression::TempDir dir;
    const QString path = dir.filePath("min.js");
    QVERIFY(regression::writeBytes(path, QByteArray(21 * 1024 * 1024, 'x')));
    const QByteArray before = sha1Of(path);
    Document doc;
    QVERIFY(doc.load(path));
    QVERIFY(doc.isMinified());
    QVERIFY(doc.isPartialView());

    QCOMPARE(doc.save(), SaveResult::PartialView);
    QCOMPARE(sha1Of(path), before);
}

PRIMEEDIT_REGRESSION_MAIN(TestT11Save)

#include "test_t1_1_save.moc"

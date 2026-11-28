// P1.11 task A regression tests (spec 2026-11-27-pr5-review-data-safety, F1 and
// F5): a direct in-place write never truncates first and reports what it left
// on disk; TextCodec::decode proves losslessness in bounded slices.
#include "support.h"

#include "core/document.h"
#include "core/textcodec.h"
#include "ui/mainwindow.h"

#include <QDir>
#include <QFile>

#include <csignal>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

using DiskState = Document::DiskState;

struct ChildSave {
    QString path;
    QString newText;
    WriteMode mode = WriteMode::Atomic;
    bool reserve = true;
};

// What the forked child saw after its save.
struct ChildReport {
    bool ran = false;
    SaveResult result = SaveResult::Ok;
    bool modified = false;
    DiskState disk = DiskState::Unchanged;
    bool recoveryPathEmpty = true;
};

constexpr int kChildSetupFailed = 0xFF;

// Runs a save in a child whose file-size limit (4096) is far below the new
// content, so the write fails the way ENOSPC does. The exit code packs bits
// 0-3 SaveResult, bit 4 isModified(), bits 5-6 DiskState, bit 7 an empty
// recoveryCopyPath().
ChildReport saveUnderFileSizeLimit(const ChildSave &job)
{
    const pid_t pid = ::fork();
    if (pid == 0) {
        Document doc;
        if (!doc.load(job.path))
            ::_exit(kChildSetupFailed);
        doc.setText(job.newText);
        Document::setReserveSpaceForTest(job.reserve);
        ::signal(SIGXFSZ, SIG_IGN);
        struct rlimit limit {4096, 4096};
        ::setrlimit(RLIMIT_FSIZE, &limit);
        const SaveResult result = doc.save(job.mode);
        ::_exit(static_cast<int>(result) | (doc.isModified() ? 0x10 : 0)
                | (static_cast<int>(doc.lastSaveDiskState()) << 5)
                | (doc.recoveryCopyPath().isEmpty() ? 0x80 : 0));
    }
    int status = 0;
    ::waitpid(pid, &status, 0);
    ChildReport report;
    if (!WIFEXITED(status) || WEXITSTATUS(status) == kChildSetupFailed)
        return report;
    const int bits = WEXITSTATUS(status);
    report.ran = true;
    report.result = static_cast<SaveResult>(bits & 0x0F);
    report.modified = bits & 0x10;
    report.disk = static_cast<DiskState>((bits >> 5) & 0x03);
    report.recoveryPathEmpty = bits & 0x80;
    return report;
}

bool hardLink(const QString &path, const QString &link)
{
    return ::link(QFile::encodeName(path).constData(), QFile::encodeName(link).constData()) == 0;
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

QByteArray lineBytes(qsizetype size)
{
    QByteArray bytes(size - 1, 'a');
    bytes.append('\n');
    return bytes;
}

// ASCII padding that ends one code unit before the slice boundary.
QString padTo(qsizetype units)
{
    return QString(units, QLatin1Char('a'));
}

struct Variants {
    QByteArray lossless;
    QByteArray altered;   // one byte changed in the final slice
    QByteArray trailing;  // one extra byte at the end
};

void checkVariants(const Variants &v, const QString &name)
{
    QVERIFY2(TextCodec::decode(v.lossless, name).lossless, qPrintable(name + " lossless"));
    QVERIFY2(!TextCodec::decode(v.altered, name).lossless, qPrintable(name + " altered"));
    QVERIFY2(!TextCodec::decode(v.trailing, name).lossless, qPrintable(name + " trailing"));
}

Variants withEnding(const QByteArray &lossless, char replacement, char extra)
{
    Variants v{lossless, lossless, lossless};
    v.altered[v.altered.size() - 1] = replacement;
    v.trailing.append(extra);
    return v;
}

// A supplementary character (two UTF-16 units) split exactly across the
// first slice boundary, then ASCII to the end.
QString splitSurrogateText(char32_t codePoint)
{
    const char32_t units[] = {codePoint};
    return padTo(TextCodec::kVerifyChunk - 1) + QString::fromUcs4(units, 1)
        + QStringLiteral("bbbb");
}

} // namespace

class TestT111aDirectWrite : public QObject
{
    Q_OBJECT

private slots:
    void reservationFailureLeavesOriginalUnchanged();
    void failedOverwriteRestoresOriginal();
    void confirmedDirectWriteKeepsOriginal();
    void failureMessageMatchesDiskState();
    void doubleFailureReportsDamaged();
    void utf8SurrogateAcrossSlice();
    void utf16SurrogateAcrossSlice();
    void iso2022EscapesAcrossSlice();
    void gb18030FourByteAcrossSlice();
    void windows1252Control();
    void declaredBomMissingIsLossy();
};

// Case 1.
void TestT111aDirectWrite::reservationFailureLeavesOriginalUnchanged()
{
    regression::TempDir dir;
    const QString path = dir.filePath("a.txt");
    const QByteArray original = lineBytes(1024);
    QVERIFY(regression::writeBytes(path, original));
    QVERIFY(hardLink(path, dir.filePath("b.txt")));

    const ChildReport r = saveUnderFileSizeLimit({path, QString(64 * 1024, QLatin1Char('x'))});
    QVERIFY2(r.ran, "child did not run the save");
    QCOMPARE(r.result, SaveResult::IoError);
    QVERIFY(r.modified);
    QCOMPARE(regression::readBytes(path), original);
    QCOMPARE(r.disk, DiskState::Unchanged);
}

// Case 2.
void TestT111aDirectWrite::failedOverwriteRestoresOriginal()
{
    regression::TempDir dir;
    const QString path = dir.filePath("a.txt");
    const QByteArray original = lineBytes(1024);
    QVERIFY(regression::writeBytes(path, original));
    QVERIFY(hardLink(path, dir.filePath("b.txt")));

    ChildSave job{path, QString(64 * 1024, QLatin1Char('x'))};
    job.reserve = false;
    const ChildReport r = saveUnderFileSizeLimit(job);
    QVERIFY2(r.ran, "child did not run the save");
    QCOMPARE(r.result, SaveResult::IoError);
    QVERIFY(r.modified);
    QCOMPARE(regression::readBytes(path), original);
    QCOMPARE(r.disk, DiskState::Restored);
}

// Case 3: the user-confirmed "Save Directly" path takes the same sequence.
void TestT111aDirectWrite::confirmedDirectWriteKeepsOriginal()
{
    regression::TempDir dir;
    const QString sub = dir.filePath("locked");
    QVERIFY(QDir().mkpath(sub));
    const QString path = sub + "/file.txt";
    const QByteArray original = lineBytes(1024);
    QVERIFY(regression::writeBytes(path, original));
    ModeGuard guard(sub, 0555);

    ChildSave job{path, QString(64 * 1024, QLatin1Char('x'))};
    job.mode = WriteMode::AllowDirect;
    const ChildReport r = saveUnderFileSizeLimit(job);
    QVERIFY2(r.ran, "child did not run the save");
    QCOMPARE(r.result, SaveResult::IoError);
    QVERIFY(r.modified);
    QCOMPARE(regression::readBytes(path), original);
}

// Case 4.
void TestT111aDirectWrite::failureMessageMatchesDiskState()
{
    const QString copy = QStringLiteral("/data/sessions/unsaved.recovered/a.txt.1234");
    const QString damagedCopy = MainWindow::saveFailureMessage(
        {"a.txt", DiskState::Damaged, copy, "No space left on device"});
    QVERIFY2(damagedCopy.contains(copy), qPrintable(damagedCopy));
    QVERIFY2(damagedCopy.contains("may be incomplete"), qPrintable(damagedCopy));
    QVERIFY2(!damagedCopy.contains("unchanged"), qPrintable(damagedCopy));

    const QString damaged = MainWindow::saveFailureMessage(
        {"a.txt", DiskState::Damaged, QString(), "No space left on device"});
    QVERIFY2(damaged.contains("may be incomplete"), qPrintable(damaged));
    QVERIFY2(!damaged.contains("unchanged"), qPrintable(damaged));

    for (DiskState state : {DiskState::Unchanged, DiskState::Restored}) {
        const QString text = MainWindow::saveFailureMessage({"a.txt", state, QString(), "err"});
        QVERIFY2(text.contains("unchanged"), qPrintable(text));
        QVERIFY2(!text.contains("may be incomplete"), qPrintable(text));
    }
}

// Case 4b: no hooks. The new text is smaller, so nothing is reserved; the
// overwrite, the restore and the 8 KiB recovery copy all hit the 4096 limit.
void TestT111aDirectWrite::doubleFailureReportsDamaged()
{
    regression::TempDir dir;
    const QString path = dir.filePath("a.txt");
    QVERIFY(regression::writeBytes(path, lineBytes(8 * 1024)));
    QVERIFY(hardLink(path, dir.filePath("b.txt")));

    const ChildReport r = saveUnderFileSizeLimit({path, QString(6 * 1024, QLatin1Char('x'))});
    QVERIFY2(r.ran, "child did not run the save");
    QCOMPARE(r.result, SaveResult::IoError);
    QCOMPARE(r.disk, DiskState::Damaged);
    QVERIFY(r.recoveryPathEmpty);
    const QString text = MainWindow::saveFailureMessage({"a.txt", r.disk, QString(), "err"});
    QVERIFY2(text.contains("may be incomplete"), qPrintable(text));
    QVERIFY2(!text.contains("unchanged"), qPrintable(text));
}

// Case 10, UTF-8 with and without a BOM.
void TestT111aDirectWrite::utf8SurrogateAcrossSlice()
{
    const QByteArray body = splitSurrogateText(U'\U0001F600').toUtf8();
    checkVariants(withEnding(body, '\xFF', '\xE2'), "UTF-8");
    const QByteArray bom("\xEF\xBB\xBF", 3);
    checkVariants(withEnding(bom + body, '\xFF', '\xE2'), "UTF-8-BOM");
}

// Case 10, UTF-16LE with and without a BOM. The altered variant turns the
// final code unit into a lone high surrogate.
void TestT111aDirectWrite::utf16SurrogateAcrossSlice()
{
    const QByteArray body = TextCodec::encode(splitSurrogateText(U'\U0001F600'), "UTF-16LE").bytes;
    checkVariants(withEnding(body, '\xD8', 'x'), "UTF-16LE");
    const QByteArray bom("\xFF\xFE", 2);
    checkVariants(withEnding(bom + body, '\xD8', 'x'), "UTF-16LE-BOM");
}

// Case 10, ISO-2022-JP: JIS mode begins two units before the boundary and
// ends exactly on it (ESC ( B is emitted by the second slice); the text ends
// in JIS mode, so only the final flush emits the closing ESC ( B.
void TestT111aDirectWrite::iso2022EscapesAcrossSlice()
{
    const QString text = padTo(TextCodec::kVerifyChunk - 2) + QStringLiteral("ああbbいう");
    const EncodeResult encoded = TextCodec::encode(text, "ISO-2022-JP");
    QVERIFY(encoded.unmappable.isEmpty());
    QVERIFY(encoded.bytes.endsWith("\x1B(B"));
    // ESC ( J decodes without error but is not what the encoder emits: only
    // the byte comparison of the flush output can see the difference.
    checkVariants(withEnding(encoded.bytes, 'J', '\x1B'), "ISO-2022-JP");
}

// Case 10, GB18030: a 4-byte supplementary character split across the boundary.
void TestT111aDirectWrite::gb18030FourByteAcrossSlice()
{
    const EncodeResult encoded = TextCodec::encode(splitSurrogateText(U'\U00020000'), "GB18030");
    QVERIFY(encoded.unmappable.isEmpty());
    checkVariants(withEnding(encoded.bytes, '\xFF', '\x81'), "GB18030");
}

// Case 10, single-byte control. ICU's windows-1252 maps all 256 bytes, so no
// windows-1252 input can be lossy: the altered and trailing variants must stay
// lossless, as the full re-encode judged them. windows-1253 leaves 0xD2
// unassigned and carries the lossy variants instead.
void TestT111aDirectWrite::windows1252Control()
{
    QByteArray bytes(TextCodec::kVerifyChunk + 16, 'a');
    bytes[TextCodec::kVerifyChunk - 1] = '\xE9';
    const Variants v = withEnding(bytes, '\x81', '\x81');
    QVERIFY(TextCodec::decode(v.lossless, "windows-1252").lossless);
    QVERIFY(TextCodec::decode(v.altered, "windows-1252").lossless);
    QVERIFY(TextCodec::decode(v.trailing, "windows-1252").lossless);
    checkVariants(withEnding(bytes, '\xD2', '\xD2'), "windows-1253");
}

// Case 10: a name declaring a UTF-8 BOM, given bytes without one.
void TestT111aDirectWrite::declaredBomMissingIsLossy()
{
    const QByteArray body = splitSurrogateText(U'\U0001F600').toUtf8();
    QVERIFY(TextCodec::decode(body, "UTF-8").lossless);
    QVERIFY(!TextCodec::decode(body, "UTF-8-BOM").lossless);
}

PRIMEEDIT_REGRESSION_MAIN(TestT111aDirectWrite)

#include "test_t1_11a_direct_write.moc"

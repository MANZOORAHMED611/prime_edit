// T1.3 regression tests (spec 2026-11-26-phase1-data-loss-fixes, D6 core):
// TextCodec is exact and reports loss, detection reads the whole file, BOMs
// survive, and a lossy decode opens read-only instead of corrupting on save.
#include "support.h"

#include "core/document.h"
#include "core/textcodec.h"

namespace {

const QByteArray kUtf16LeBom("\xFF\xFE", 2);

QByteArray utf16Le(const QString &text)
{
    return QByteArray(reinterpret_cast<const char *>(text.utf16()), text.size() * 2);
}

// Restores the production size thresholds even when a test fails early.
struct ThresholdGuard {
    ThresholdGuard(qint64 medium, qint64 large) { Document::setSizeThresholdsForTest(medium, large); }
    ~ThresholdGuard()
    {
        Document::setSizeThresholdsForTest(Document::MEDIUM_FILE_THRESHOLD,
                                           Document::LARGE_FILE_THRESHOLD);
    }
};

} // namespace

class TestT13Encoding : public QObject
{
    Q_OBJECT

private slots:
    void codecReportsUnmappablePositions();
    void codecLosslessMeansExactRoundTrip();
    void codecNamesFollowTheTable();
    void nonUtf8ByteAfter8KbIsDetected();
    void mediumModeDecodesUtf16();
    void lossyDecodeOpensReadOnly();
    void reloadWithEncodingRecoversWindows1252();
    void bomKeptOnSave();
    void cp932ManyToOneOpensReadOnly();
    void encodeLossyLeavesFileUntouched();
};

void TestT13Encoding::codecReportsUnmappablePositions()
{
    const QString text = QString::fromUtf8("aب cت");
    const EncodeResult latin = TextCodec::encode(text, "ISO-8859-1");
    QCOMPARE(latin.unmappable, (QList<int>{1, 4}));

    const EncodeResult arabic = TextCodec::encode(text, "windows-1256");
    QVERIFY(arabic.unmappable.isEmpty());
    QCOMPARE(arabic.bytes, QByteArray("a\xC8 c\xCA"));

    QString loneSurrogate = QStringLiteral("ab");
    loneSurrogate.insert(1, QChar(0xD800));
    QCOMPARE(TextCodec::encode(loneSurrogate, "UTF-8").unmappable, QList<int>{1});
}

void TestT13Encoding::codecLosslessMeansExactRoundTrip()
{
    const DecodeResult latin = TextCodec::decode("caf\xE9", "windows-1252");
    QVERIFY(latin.lossless);
    QCOMPARE(latin.text, QString::fromUtf8("café"));

    QVERIFY(!TextCodec::decode("ab\xFF", "UTF-8").lossless);
    QVERIFY(!TextCodec::decode("\xA1", "ISO-8859-6").lossless);

    // A BOM the name does not declare is kept as U+FEFF, so nothing is dropped.
    const DecodeResult undeclared = TextCodec::decode(kUtf16LeBom + utf16Le("hi"), "UTF-16LE");
    QVERIFY(undeclared.lossless);
    QCOMPARE(undeclared.text, QString(QChar(0xFEFF)) + "hi");
    const DecodeResult declared = TextCodec::decode(kUtf16LeBom + utf16Le("hi"), "UTF-16LE-BOM");
    QVERIFY(declared.lossless);
    QCOMPARE(declared.text, QString("hi"));
    QCOMPARE(TextCodec::encode("hi", "UTF-16LE-BOM").bytes, kUtf16LeBom + utf16Le("hi"));
}

// Spec 9.1 encoding-name table: aliases resolve to the table's spelling.
void TestT13Encoding::codecNamesFollowTheTable()
{
    regression::TempDir dir;
    const QString path = dir.filePath("ascii.txt");
    QVERIFY(regression::writeBytes(path, "ascii only\n"));
    Document doc;
    QVERIFY(doc.load(path));
    QCOMPARE(doc.encoding(), QString("UTF-8"));

    const QList<QPair<QString, QString>> table{
        {"Windows-1252", "windows-1252"}, {"Shift-JIS", "Shift_JIS"},
        {"latin1", "ISO-8859-1"},         {"ISO-8859-6", "ISO-8859-6"},
        {"cp1256", "windows-1256"},       {"utf-16be-bom", "UTF-16BE-BOM"},
        {"UTF-8-BOM", "UTF-8-BOM"},
    };
    for (const auto &entry : table) {
        QVERIFY2(doc.reloadWithEncoding(entry.first), qPrintable(entry.first));
        QCOMPARE(doc.encoding(), entry.second);
    }
}

void TestT13Encoding::nonUtf8ByteAfter8KbIsDetected()
{
    regression::TempDir dir;
    const QString path = dir.filePath("late.txt");
    const QByteArray bytes = QByteArray(9000, 'a') + "\ncaf\xE9 au lait\n";
    QVERIFY(regression::writeBytes(path, bytes));
    Document doc;
    QVERIFY(doc.load(path));

    QCOMPARE(doc.encoding(), QString("ISO-8859-1"));
    QVERIFY(!doc.wasDecodedLossily());
    QVERIFY(doc.text().endsWith(QString::fromUtf8("café au lait\n")));
    QCOMPARE(doc.save(), SaveResult::Ok);
    QCOMPARE(regression::readBytes(path), bytes);
}

void TestT13Encoding::mediumModeDecodesUtf16()
{
    ThresholdGuard guard(1024, 1024 * 1024);
    regression::TempDir dir;
    const QString path = dir.filePath("medium16.txt");
    QString text;
    for (int i = 0; i < 200; ++i)
        text += QString::fromUtf8("line %1 العربية\n").arg(i);
    const QByteArray bytes = kUtf16LeBom + utf16Le(text);
    QVERIFY(regression::writeBytes(path, bytes));
    Document doc;
    QVERIFY(doc.load(path));

    QCOMPARE(doc.fileMode(), Document::MediumFile);
    QCOMPARE(doc.encoding(), QString("UTF-16LE-BOM"));
    QCOMPARE(doc.text(), text);
    QCOMPARE(doc.save(), SaveResult::Ok);
    QCOMPARE(regression::readBytes(path), bytes);
}

void TestT13Encoding::lossyDecodeOpensReadOnly()
{
    regression::TempDir dir;
    const QString path = dir.filePath("bad-arabic.txt");
    // Valid ISO-8859-6 except 0xA1, which the charset does not assign.
    const QByteArray bytes("\xC7\xE4\xD9\xD1\xC8\xEA\xC9 \xA1\n");
    QVERIFY(regression::writeBytes(path, bytes));
    Document doc;
    QVERIFY(doc.load(path));
    QVERIFY(doc.reloadWithEncoding("ISO-8859-6"));

    QVERIFY(doc.wasDecodedLossily());
    QVERIFY(doc.isReadOnly());
    QCOMPARE(doc.save(), SaveResult::ReadOnly);
    QCOMPARE(regression::readBytes(path), bytes);
}

void TestT13Encoding::reloadWithEncodingRecoversWindows1252()
{
    regression::TempDir dir;
    const QString path = dir.filePath("cp1252.txt");
    // 0x93/0x94 are curly quotes in windows-1252 and undefined in UTF-8.
    const QByteArray bytes("\x93quoted\x94 \x80 5\r\n");
    QVERIFY(regression::writeBytes(path, bytes));
    Document doc;
    QVERIFY(doc.load(path));
    QVERIFY(!doc.reloadWithEncoding("no-such-codec"));

    QVERIFY(doc.reloadWithEncoding("windows-1252"));
    QCOMPARE(doc.encoding(), QString("windows-1252"));
    QVERIFY(!doc.wasDecodedLossily());
    QVERIFY(!doc.isReadOnly());
    QVERIFY(!doc.isModified());
    QCOMPARE(doc.text(), QString::fromUtf8("“quoted” € 5\n"));
    QCOMPARE(doc.save(), SaveResult::Ok);
    QCOMPARE(regression::readBytes(path), bytes);
}

void TestT13Encoding::bomKeptOnSave()
{
    regression::TempDir dir;
    const QString path = dir.filePath("bom16.txt");
    const QByteArray bytes = kUtf16LeBom + utf16Le("first\nsecond\n");
    QVERIFY(regression::writeBytes(path, bytes));
    Document doc;
    QVERIFY(doc.load(path));

    QCOMPARE(doc.encoding(), QString("UTF-16LE-BOM"));
    QCOMPARE(doc.text(), QString("first\nsecond\n"));
    doc.insert(0, "x");
    doc.remove(0, 1);
    QCOMPARE(doc.save(), SaveResult::Ok);
    QCOMPARE(regression::readBytes(path), bytes);
}

void TestT13Encoding::cp932ManyToOneOpensReadOnly()
{
    regression::TempDir dir;
    const QString path = dir.filePath("cp932.txt");
    // 0xED40 decodes to U+7E8A, which Shift_JIS encodes back as 0xFA5C.
    const QByteArray bytes("abc \xED\x40 def\n");
    QVERIFY(regression::writeBytes(path, bytes));
    Document doc;
    QVERIFY(doc.load(path));

    QCOMPARE(doc.encoding(), QString("Shift_JIS"));
    QVERIFY(doc.wasDecodedLossily());
    QVERIFY(doc.isReadOnly());
    QCOMPARE(doc.save(), SaveResult::ReadOnly);
    QCOMPARE(regression::readBytes(path), bytes);
}

void TestT13Encoding::encodeLossyLeavesFileUntouched()
{
    regression::TempDir dir;
    const QString path = dir.filePath("latin.txt");
    const QByteArray bytes("plain caf\xE9\n");
    QVERIFY(regression::writeBytes(path, bytes));
    Document doc;
    QVERIFY(doc.load(path));
    QCOMPARE(doc.encoding(), QString("ISO-8859-1"));

    doc.insert(0, QString::fromUtf8("ب"));
    QCOMPARE(doc.save(), SaveResult::EncodeLossy);
    QVERIFY(!doc.lastError().isEmpty());
    QVERIFY(doc.isModified());
    QCOMPARE(regression::readBytes(path), bytes);
}

PRIMEEDIT_REGRESSION_MAIN(TestT13Encoding)

#include "test_t1_3_encoding.moc"

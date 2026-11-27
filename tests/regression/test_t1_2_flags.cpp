// T1.2 regression tests (spec 2026-11-26-phase1-data-loss-fixes, D1 core, D9,
// D11 core): a pending conversion keeps the document modified, a file with no
// line break takes the configured default, and mixed line endings are recorded.
#include "support.h"

#include "core/document.h"
#include "ui/editor.h"
#include "utils/settings.h"

#include <QTextCursor>

#include <sys/stat.h>

namespace {

// Types into the editor and undoes it, so the QTextDocument returns to its
// clean state and reports modificationChanged(false) to the Document.
void typeAndUndo(Editor &editor)
{
    QTextCursor cursor = editor.textCursor();
    cursor.movePosition(QTextCursor::End);
    editor.setTextCursor(cursor);
    // Editor::document() returns the Document; the text flag is on QTextDocument.
    QTextDocument *textDoc = editor.QPlainTextEdit::document();
    QTest::keyClick(&editor, Qt::Key_X);
    QVERIFY(textDoc->isModified());
    editor.undo();
    QVERIFY(!textDoc->isModified());
}

} // namespace

class TestT12Flags : public QObject
{
    Q_OBJECT

private slots:
    void encodingConversionStaysModifiedAfterTypeAndUndo();
    void lineEndingConversionStaysModifiedAfterTypeAndUndo();
    void formatDirtyClearedOnlyBySuccessfulSaveOrLoad();
    void emptyFileTypedLinesSaveAsLf();
    void noLineBreakFollowsDefaultSetting();
    void mixedLineEndingsRecordedAndNormalisedToMajority();
};

void TestT12Flags::encodingConversionStaysModifiedAfterTypeAndUndo()
{
    regression::TempDir dir;
    const QString path = dir.filePath("enc.txt");
    QVERIFY(regression::writeBytes(path, "hello\n"));
    Document doc;
    QVERIFY(doc.load(path));
    Editor editor(&doc);
    QVERIFY(!doc.isModified());

    doc.setEncoding("UTF-16LE");
    QVERIFY(doc.isModified());
    typeAndUndo(editor);
    QVERIFY2(doc.isModified(), "undo of a text edit must not hide a pending conversion");
    QVERIFY(doc.isFormatDirty());
}

void TestT12Flags::lineEndingConversionStaysModifiedAfterTypeAndUndo()
{
    regression::TempDir dir;
    const QString path = dir.filePath("eol.txt");
    QVERIFY(regression::writeBytes(path, "one\ntwo\n"));
    Document doc;
    QVERIFY(doc.load(path));
    Editor editor(&doc);
    QSignalSpy spy(&doc, &Document::modifiedChanged);

    doc.setLineEnding(Document::Windows);
    QVERIFY(doc.isModified());
    typeAndUndo(editor);
    QVERIFY(doc.isModified());
    // The flag never flipped back to clean, so listeners saw exactly one change.
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toBool(), true);
}

void TestT12Flags::formatDirtyClearedOnlyBySuccessfulSaveOrLoad()
{
    regression::TempDir dir;
    const QString path = dir.filePath("flags.txt");
    QVERIFY(regression::writeBytes(path, "a\nb\n"));
    Document doc;
    QVERIFY(doc.load(path));

    doc.setLineEnding(Document::Windows);
    doc.setModified(false);
    QVERIFY(doc.isFormatDirty());
    QVERIFY(doc.isModified());

    QVERIFY(::chmod(QFile::encodeName(path).constData(), 0444) == 0);
    QCOMPARE(doc.save(), SaveResult::IoError);
    QVERIFY2(doc.isFormatDirty(), "a failed save must not clear the pending conversion");
    QVERIFY(::chmod(QFile::encodeName(path).constData(), 0644) == 0);
    QCOMPARE(doc.save(), SaveResult::Ok);
    QVERIFY(!doc.isFormatDirty());
    QVERIFY(!doc.isModified());
    QCOMPARE(regression::readBytes(path), QByteArray("a\r\nb\r\n"));

    doc.setEncoding("UTF-16LE");
    QVERIFY(doc.isFormatDirty());
    QVERIFY(doc.load(path));
    QVERIFY(!doc.isFormatDirty());
    QVERIFY(!doc.isModified());
}

void TestT12Flags::emptyFileTypedLinesSaveAsLf()
{
    regression::TempDir dir;
    const QString path = dir.filePath("empty.txt");
    QVERIFY(regression::writeBytes(path, QByteArray("")));
    Document doc;
    QVERIFY(doc.load(path));
    QCOMPARE(doc.lineEnding(), Document::Unix);
    Editor editor(&doc);

    QTest::keyClick(&editor, Qt::Key_A);
    QTest::keyClick(&editor, Qt::Key_Return);
    QTest::keyClick(&editor, Qt::Key_B);
    editor.syncToDocument();
    QCOMPARE(doc.save(), SaveResult::Ok);
    QCOMPARE(regression::readBytes(path).toHex(' '), QByteArray("61 0a 62"));
}

void TestT12Flags::noLineBreakFollowsDefaultSetting()
{
    regression::TempDir dir;
    const QString path = dir.filePath("oneline.txt");
    QVERIFY(regression::writeBytes(path, "no newline here"));
    Document doc;
    QVERIFY(doc.load(path));
    QCOMPARE(doc.lineEnding(), Document::Unix);
    QVERIFY(!doc.hadMixedLineEndings());

    Settings &settings = Settings::instance();
    const QString previous = settings.defaultLineEnding();
    settings.setDefaultLineEnding("CRLF");
    Document crlfDoc;
    const bool loaded = crlfDoc.load(path);
    settings.setDefaultLineEnding(previous);
    QVERIFY(loaded);
    QCOMPARE(crlfDoc.lineEnding(), Document::Windows);
}

void TestT12Flags::mixedLineEndingsRecordedAndNormalisedToMajority()
{
    regression::TempDir dir;
    const QString uniform = dir.filePath("uniform.txt");
    const QString mixed = dir.filePath("mixed.txt");
    QVERIFY(regression::writeBytes(uniform, "a\r\nb\r\n"));
    QVERIFY(regression::writeBytes(mixed, "a\r\nb\r\nc\n"));

    Document uniformDoc;
    QVERIFY(uniformDoc.load(uniform));
    QVERIFY(!uniformDoc.hadMixedLineEndings());

    Document doc;
    QVERIFY(doc.load(mixed));
    QVERIFY(doc.hadMixedLineEndings());
    QCOMPARE(doc.lineEnding(), Document::Windows);
    QCOMPARE(doc.save(), SaveResult::Ok);
    QCOMPARE(regression::readBytes(mixed), QByteArray("a\r\nb\r\nc\r\n"));
}

PRIMEEDIT_REGRESSION_MAIN(TestT12Flags)

#include "test_t1_2_flags.moc"

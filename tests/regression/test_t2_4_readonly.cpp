// T2.4 (spec D3 editor side, D12 editor side): the Editor mirrors
// Document::isReadOnly() at bind time and on readOnlyChanged, and refuses every
// keystroke while read-only. A partial view gets no banner text in its buffer.
#include "support.h"

#include "core/document.h"
#include "ui/editor.h"

#include <QClipboard>
#include <QTextDocument>

namespace {

constexpr qint64 kMiB = 1024 * 1024;

QTextDocument *textDocument(Editor *editor)
{
    return editor->QPlainTextEdit::document();
}

// Writes size bytes of lines, so the file is a normal (not minified) text file.
bool writeLinesFile(const QString &path, qint64 size)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    const QByteArray line = QByteArray(79, 'a') + '\n';
    const QByteArray chunk = line.repeated(int(kMiB / line.size()));
    for (qint64 written = 0; written < size; written += chunk.size()) {
        if (file.write(chunk) != chunk.size())
            return false;
    }
    return true;
}

// Writes size bytes with no line break at all: a minified file.
bool writeMinifiedFile(const QString &path, qint64 size)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    const QByteArray chunk(int(kMiB), 'm');
    for (qint64 written = 0; written < size; written += chunk.size()) {
        if (file.write(chunk) != chunk.size())
            return false;
    }
    return true;
}

// Every key class Editor::keyPressEvent handles itself: plain text, Enter
// (auto-indent), Tab (indent), an auto-closed bracket, Backspace, Delete and
// paste.
void typeEditingKeys(Editor *editor)
{
    QApplication::clipboard()->setText(QStringLiteral("pasted"));
    editor->moveCursor(QTextCursor::Start);
    QTest::keyClicks(editor, QStringLiteral("typed"));
    QTest::keyClick(editor, Qt::Key_Return);
    QTest::keyClick(editor, Qt::Key_Tab);
    QTest::keyClicks(editor, QStringLiteral("("));
    QTest::keyClick(editor, Qt::Key_Delete);
    editor->moveCursor(QTextCursor::End);
    QTest::keyClick(editor, Qt::Key_Backspace);
    QTest::keyClick(editor, Qt::Key_V, Qt::ControlModifier);
}

} // namespace

class TestT24ReadOnly : public QObject
{
    Q_OBJECT

private:
    void checkKeystrokesRefused(Editor *editor, Document *doc)
    {
        QVERIFY2(editor->isReadOnly(), "the editor does not mirror Document::isReadOnly()");
        const QString before = editor->rawText();
        typeEditingKeys(editor);
        QCOMPARE(editor->rawText(), before);
        QVERIFY(!doc->isModified());
        QVERIFY(!textDocument(editor)->isModified());

        // Still readable from the keyboard: arrows move, Shift selects.
        editor->moveCursor(QTextCursor::Start);
        QTest::keyClick(editor, Qt::Key_Right);
        QCOMPARE(editor->textCursor().position(), 1);
        QTest::keyClick(editor, Qt::Key_Right, Qt::ShiftModifier);
        QCOMPARE(editor->textCursor().selectedText().size(), 1);
    }

    void checkPartialView(const QString &path)
    {
        Document doc;
        QVERIFY(doc.load(path));
        QVERIFY(doc.isPartialView());
        Editor editor(&doc);
        editor.show();

        QVERIFY2(!editor.rawText().contains(QStringLiteral("Showing first")),
                 "banner text was appended to the partial view's buffer");
        checkKeystrokesRefused(&editor, &doc);
    }

private slots:
    void partialView51MiBRefusesEdits()
    {
        regression::TempDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("large.txt"));
        QVERIFY(writeLinesFile(path, 51 * kMiB));
        checkPartialView(path);
    }

    void partialViewMinified21MiBRefusesEdits()
    {
        regression::TempDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("bundle.min.js"));
        QVERIFY(writeMinifiedFile(path, 21 * kMiB));
        checkPartialView(path);
    }

    void readOnlyAfterBindingRefusesEdits()
    {
        Document doc;
        doc.setText(QStringLiteral("line one\nline two\n"));
        doc.setModified(false);
        Editor editor(&doc);
        editor.show();
        QVERIFY(!editor.isReadOnly());

        doc.setReadOnly(true);
        checkKeystrokesRefused(&editor, &doc);

        doc.setReadOnly(false);
        QVERIFY2(!editor.isReadOnly(), "the editor stayed read-only after readOnlyChanged(false)");
        editor.moveCursor(QTextCursor::Start);
        QTest::keyClicks(&editor, QStringLiteral("x"));
        QVERIFY(editor.rawText().startsWith(QLatin1Char('x')));
        QVERIFY(doc.isModified());
    }
};

PRIMEEDIT_REGRESSION_MAIN(TestT24ReadOnly)

#include "test_t2_4_readonly.moc"

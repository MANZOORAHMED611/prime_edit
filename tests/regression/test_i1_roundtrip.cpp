// T4.1: the I1 round-trip matrix (spec docs/superpowers/specs/2026-11-26-phase1-data-loss-fixes.md,
// section 5 "I1 round-trip matrix", section 9.2 row T4.1).
//
// Every case goes through the real UI path: MainWindow::openFile, type one
// character, delete it, MainWindow::saveFile, then compare the file's bytes
// with the bytes written. Fixtures are generated here. Legacy fixtures are
// hard-coded bytes (not produced by TextCodec), so a codec bug cannot make
// the fixture agree with itself.
//
// Detection scope (W1, documented in html-docs/guide.html and faq.html):
// BOM-less UTF-16, ISO-8859-6 and windows-1256 are not recognised. For those
// the detection pass asserts byte identity only, and the encoding name and
// decoded text are asserted after Reopen with Encoding.
#include "support.h"

#include "core/document.h"
#include "ui/editor.h"
#include "ui/mainwindow.h"
#include "ui/notificationbar.h"

#include <QAction>
#include <QDialog>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QStringConverter>
#include <QTimer>

namespace {

// Closes any modal dialog that opens while it is alive and records its title,
// so an unexpected prompt fails the case instead of hanging the suite.
class ModalGuard
{
public:
    ModalGuard()
    {
        QObject::connect(&m_timer, &QTimer::timeout, [this]() {
            auto *dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if (!dialog) return;
            m_titles << dialog->windowTitle();
            dialog->done(QDialog::Rejected);
        });
        m_timer.start(20);
    }
    QStringList titles() const { return m_titles; }

private:
    QTimer m_timer;
    QStringList m_titles;
};

// Picks one item in the next QInputDialog (Reopen with Encoding) and accepts it.
class InputPicker
{
public:
    explicit InputPicker(const QString &choice)
    {
        QObject::connect(&m_timer, &QTimer::timeout, [this, choice]() {
            auto *dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget());
            if (!dialog) return;
            m_offered = dialog->comboBoxItems().contains(choice);
            dialog->setTextValue(choice);
            ++m_shown;
            dialog->accept();
        });
        m_timer.start(20);
    }
    int shown() const { return m_shown; }
    bool offered() const { return m_offered; }

private:
    QTimer m_timer;
    int m_shown = 0;
    bool m_offered = false;
};

// Restores the production size thresholds even when a QVERIFY returns early.
class MediumModeScope
{
public:
    MediumModeScope() { Document::setSizeThresholdsForTest(0, Document::LARGE_FILE_THRESHOLD); }
    ~MediumModeScope()
    {
        Document::setSizeThresholdsForTest(Document::MEDIUM_FILE_THRESHOLD, Document::LARGE_FILE_THRESHOLD);
    }
};

struct Ending {
    const char *name;
    const char *bytes;
    Document::LineEnding kind;
};

const Ending kEndings[] = {
    {"LF", "\n", Document::Unix},
    {"CRLF", "\r\n", Document::Windows},
    {"CR", "\r", Document::ClassicMac},
};

// Two lines, each terminated, so the file has a line ending to detect and keep.
QByteArray twoLines(const QByteArray &line, const Ending &e)
{
    return line + e.bytes + line + e.bytes;
}

QString twoLinesText(const QString &line) { return line + QLatin1Char('\n') + line + QLatin1Char('\n'); }

QByteArray utfBytes(const QString &text, QStringConverter::Encoding enc, bool bom)
{
    QStringEncoder encoder(enc, bom ? QStringConverter::Flag::WriteBom : QStringConverter::Flag::Default);
    return encoder.encode(text);
}

// The sample line for the UTF family: Latin-1 range, an em dash, Arabic, CJK
// and a non-BMP character (a surrogate pair in UTF-16).
QString utfLine() { return QStringLiteral("Grüße — مرحبا 日本 😀"); }

struct UtfCase {
    const char *label;
    QStringConverter::Encoding enc;
    bool bom;
    const char *detected;  // empty: not recognised by detection
    const char *reopenAs;  // empty: no Reopen pass
};

const UtfCase kUtfCases[] = {
    {"UTF-8", QStringConverter::Utf8, false, "UTF-8", ""},
    {"UTF-8-BOM", QStringConverter::Utf8, true, "UTF-8-BOM", ""},
    {"UTF-16LE", QStringConverter::Utf16LE, false, "", "UTF-16LE"},
    {"UTF-16LE-BOM", QStringConverter::Utf16LE, true, "UTF-16LE-BOM", ""},
    {"UTF-16BE", QStringConverter::Utf16BE, false, "", "UTF-16BE"},
    {"UTF-16BE-BOM", QStringConverter::Utf16BE, true, "UTF-16BE-BOM", ""},
    {"UTF-32LE-BOM", QStringConverter::Utf32LE, true, "UTF-32LE-BOM", ""},
};

struct LegacyCase {
    const char *name;      // canonical name, used for Reopen
    const char *line;      // the line's bytes in this encoding
    const char *text;      // the same line as UTF-8 source text
    bool detectable;       // in detection scope
};

const LegacyCase kLegacyCases[] = {
    {"windows-1252", "\x93quoted\x94 \x80 5", "“quoted” € 5", true},
    // JIS X 0208 kanji whose bytes avoid 0x80-0x9F, plus half-width katakana,
    // so the detector's windows-1252 rule does not claim them first.
    {"Shift_JIS", "\xE0\x40\xE0\x41\xE0\x42\xB1\xB2\xB3", "漾漓滷ｱｲｳ", true},
    {"ISO-8859-6", "\xE5\xD1\xCD\xC8\xC7", "مرحبا", false},
    {"windows-1256", "\xE3\xD1\xCD\xC8\xC7", "مرحبا", false},
};

void addColumns()
{
    QTest::addColumn<QByteArray>("bytes");
    QTest::addColumn<QString>("encoding");  // expected name; empty = not asserted
    QTest::addColumn<QString>("text");      // expected rawText(); null = not asserted
    QTest::addColumn<int>("ending");        // expected Document::LineEnding; -1 = not asserted
}

void addRow(const QString &name, const QByteArray &bytes, const QString &encoding,
            const QString &text, int ending)
{
    QTest::newRow(qPrintable(name)) << bytes << encoding << text << ending;
}

QString notificationText(MainWindow &mw)
{
    auto *bar = mw.findChild<NotificationBar*>();
    if (!bar || !bar->isVisible()) return QString();
    QStringList parts;
    for (QLabel *label : bar->findChildren<QLabel*>()) parts << label->text();
    return parts.join(QLatin1Char(' '));
}

// The Encoding > Reopen with Encoding... menu item, found by its label so the
// case drives exactly what the user clicks (it calls MainWindow::reopenWithEncoding).
QAction *reopenAction(MainWindow &mw)
{
    for (QAction *top : mw.menuBar()->actions()) {
        if (top->text().remove(QLatin1Char('&')) != QStringLiteral("Encoding") || !top->menu()) continue;
        for (QAction *a : top->menu()->actions()) {
            if (a->text().remove(QLatin1Char('&')).startsWith(QStringLiteral("Reopen with Encoding"))) return a;
        }
    }
    return nullptr;
}

bool showActive(QWidget &w)
{
    w.show();
    w.activateWindow();
    return QTest::qWaitForWindowActive(&w);
}

} // namespace

class TestI1Roundtrip : public QObject
{
    Q_OBJECT

private slots:
    void byDetection_data();
    void byDetection();
    void byDetectionMediumMode_data();
    void byDetectionMediumMode();
    void byReopenWithEncoding_data();
    void byReopenWithEncoding();
    void mixedLineEndingsNormaliseToMajorityWithNotice_data();
    void mixedLineEndingsNormaliseToMajorityWithNotice();

private:
    void runDetectionCase();
    void typeDeleteSave(MainWindow &mw, Editor *editor);
    void checkLoaded(Editor *editor, const QString &encoding, const QString &text, int ending);
};

// Every fixture opened by detection. An out-of-scope fixture must still come
// back byte-identical; if it opened read-only (lossy decode), the save must be
// refused and leave the bytes alone.
void TestI1Roundtrip::byDetection_data()
{
    addColumns();
    for (const Ending &e : kEndings) {
        for (const UtfCase &u : kUtfCases) {
            addRow(QStringLiteral("%1/%2").arg(u.label, e.name),
                   utfBytes(utfLine() + QString::fromLatin1(e.bytes) + utfLine() + QString::fromLatin1(e.bytes),
                            u.enc, u.bom),
                   QString::fromLatin1(u.detected), *u.detected ? twoLinesText(utfLine()) : QString(),
                   *u.detected ? int(e.kind) : -1);
        }
        for (const LegacyCase &l : kLegacyCases) {
            addRow(QStringLiteral("%1/%2").arg(l.name, e.name), twoLines(l.line, e),
                   l.detectable ? QString::fromLatin1(l.name) : QString(),
                   l.detectable ? twoLinesText(QString::fromUtf8(l.text)) : QString(),
                   l.detectable ? int(e.kind) : -1);
        }
    }
    addRow("empty", QByteArray(""), "UTF-8", QStringLiteral(""), Document::Unix);
    addRow("no-newline", "abc", "UTF-8", QStringLiteral("abc"), Document::Unix);
    addRow("U+00A0/LF", "a\xC2\xA0" "b\n", "UTF-8", QStringLiteral("a b\n"), Document::Unix);
    addRow("U+2028/LF", "a\xE2\x80\xA8" "b\n", "UTF-8", QStringLiteral("a b\n"), Document::Unix);
    addRow("lone-CR-terminated", "abc\r", "UTF-8", QStringLiteral("abc\n"), Document::ClassicMac);
    addRow("lone-CR-only", "\r", "UTF-8", QStringLiteral("\n"), Document::ClassicMac);
}

void TestI1Roundtrip::byDetection()
{
    runDetectionCase();
}

// The same matrix with every non-empty fixture loaded in Medium mode.
void TestI1Roundtrip::byDetectionMediumMode_data()
{
    byDetection_data();
}

void TestI1Roundtrip::byDetectionMediumMode()
{
    MediumModeScope medium;
    runDetectionCase();
}

void TestI1Roundtrip::runDetectionCase()
{
    QFETCH(QByteArray, bytes);
    QFETCH(QString, encoding);
    QFETCH(QString, text);
    QFETCH(int, ending);
    regression::TempDir tmp;
    QVERIFY(tmp.isValid());
    const QString file = tmp.filePath(QStringLiteral("fixture.txt"));
    QVERIFY(regression::writeBytes(file, bytes));

    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.openFile(file);
    QVERIFY(editor);
    if (encoding.isEmpty() && editor->document()->isReadOnly()) {
        // Out of detection scope and decoded lossily: D6 opens it read-only.
        QVERIFY(editor->document()->wasDecodedLossily());
        ModalGuard guard;
        mw.saveFile();
        QCOMPARE(regression::readBytes(file), bytes);
        return;
    }
    checkLoaded(editor, encoding, text, ending);
    if (QTest::currentTestFailed()) return;
    typeDeleteSave(mw, editor);
    if (QTest::currentTestFailed()) return;
    QCOMPARE(regression::readBytes(file), bytes);
}

// Legacy fixtures and BOM-less UTF-16, decoded again with the right encoding.
void TestI1Roundtrip::byReopenWithEncoding_data()
{
    addColumns();
    for (const Ending &e : kEndings) {
        for (const UtfCase &u : kUtfCases) {
            if (!*u.reopenAs) continue;
            addRow(QStringLiteral("%1/%2").arg(u.label, e.name),
                   utfBytes(utfLine() + QString::fromLatin1(e.bytes) + utfLine() + QString::fromLatin1(e.bytes),
                            u.enc, false),
                   QString::fromLatin1(u.reopenAs), twoLinesText(utfLine()), int(e.kind));
        }
        for (const LegacyCase &l : kLegacyCases) {
            addRow(QStringLiteral("%1/%2").arg(l.name, e.name), twoLines(l.line, e),
                   QString::fromLatin1(l.name), twoLinesText(QString::fromUtf8(l.text)), int(e.kind));
        }
    }
}

void TestI1Roundtrip::byReopenWithEncoding()
{
    QFETCH(QByteArray, bytes);
    QFETCH(QString, encoding);
    QFETCH(QString, text);
    QFETCH(int, ending);
    regression::TempDir tmp;
    QVERIFY(tmp.isValid());
    const QString file = tmp.filePath(QStringLiteral("fixture.txt"));
    QVERIFY(regression::writeBytes(file, bytes));

    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.openFile(file);
    QVERIFY(editor);
    QAction *reopen = reopenAction(mw);
    QVERIFY2(reopen, "the Encoding menu has no Reopen with Encoding item");
    QVERIFY(reopen->isEnabled());
    {
        InputPicker pick(encoding);
        reopen->trigger();
        QCOMPARE(pick.shown(), 1);
        QVERIFY2(pick.offered(), qPrintable(encoding + QStringLiteral(" is not offered")));
    }
    checkLoaded(editor, encoding, text, ending);
    if (QTest::currentTestFailed()) return;
    typeDeleteSave(mw, editor);
    if (QTest::currentTestFailed()) return;
    QCOMPARE(regression::readBytes(file), bytes);
}

// D11, the documented exception to I1: mixed endings are saved as the
// majority ending, and the notification bar says so.
void TestI1Roundtrip::mixedLineEndingsNormaliseToMajorityWithNotice_data()
{
    QTest::addColumn<QByteArray>("bytes");
    QTest::addColumn<QByteArray>("expected");
    QTest::addColumn<QString>("majority");
    QTest::newRow("CRLF-majority") << QByteArray("a\r\nb\r\nc\n") << QByteArray("a\r\nb\r\nc\r\n") << QStringLiteral("CRLF");
    QTest::newRow("LF-majority") << QByteArray("a\nb\nc\r\n") << QByteArray("a\nb\nc\n") << QStringLiteral("LF");
}

void TestI1Roundtrip::mixedLineEndingsNormaliseToMajorityWithNotice()
{
    QFETCH(QByteArray, bytes);
    QFETCH(QByteArray, expected);
    QFETCH(QString, majority);
    regression::TempDir tmp;
    const QString file = tmp.filePath(QStringLiteral("mixed.txt"));
    QVERIFY(regression::writeBytes(file, bytes));

    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.openFile(file);
    QVERIFY(editor);
    QVERIFY(editor->document()->hadMixedLineEndings());
    typeDeleteSave(mw, editor);
    if (QTest::currentTestFailed()) return;
    QCOMPARE(regression::readBytes(file), expected);
    const QString notice = notificationText(mw);
    QVERIFY2(notice.contains(QStringLiteral("Mixed line endings")) && notice.contains(majority),
             qPrintable(notice));
}

void TestI1Roundtrip::checkLoaded(Editor *editor, const QString &encoding, const QString &text, int ending)
{
    Document *doc = editor->document();
    QVERIFY2(!editor->isReadOnly(), "the fixture opened read-only, so it cannot be edited");
    QVERIFY(!doc->wasDecodedLossily());
    if (!encoding.isEmpty()) QCOMPARE(doc->encoding(), encoding);
    if (!text.isNull()) QCOMPARE(editor->rawText(), text);
    if (ending >= 0) QCOMPARE(int(doc->lineEnding()), ending);
}

// Type one character at the start, delete it, and save through the menu slot.
void TestI1Roundtrip::typeDeleteSave(MainWindow &mw, Editor *editor)
{
    editor->setFocus();
    editor->moveCursor(QTextCursor::Start);
    QTest::keyClicks(editor, QStringLiteral("x"));
    QTest::keyClick(editor, Qt::Key_Backspace);
    QVERIFY2(editor->document()->isModified(), "type plus delete did not mark the document modified");

    ModalGuard guard;
    mw.saveFile();
    QVERIFY2(guard.titles().isEmpty(), qPrintable(QStringLiteral("unexpected dialog: ") + guard.titles().join(", ")));
    QVERIFY2(!editor->document()->isModified(), "the save did not complete (document still modified)");
}

PRIMEEDIT_REGRESSION_MAIN(TestI1Roundtrip)

#include "test_i1_roundtrip.moc"

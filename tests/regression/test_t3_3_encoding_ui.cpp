// T3.3 regression tests (spec docs/superpowers/specs/2026-11-26-phase1-data-loss-fixes.md,
// D6/D11 UI, section 9.2 row T3.3). Every test drives a real MainWindow.
#include "support.h"

#include "core/document.h"
#include "core/documentmanager.h"
#include "ui/editor.h"
#include "ui/mainwindow.h"
#include "ui/notificationbar.h"
#include "ui/searchdialog.h"
#include "ui/statusbar.h"

#include <QAbstractButton>
#include <QAction>
#include <QComboBox>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>

namespace {

// Answers every QMessageBox that opens while it is alive by clicking the button
// whose text contains wantedText (or the wanted standard button). A missing
// button is recorded as a miss and the box is closed, so a test fails instead
// of hanging. Each answered box's text is kept for assertions.
class PromptAnswerer
{
public:
    explicit PromptAnswerer(QMessageBox::StandardButton wanted) : m_wanted(wanted) { start(); }
    explicit PromptAnswerer(const QString &wantedText) : m_wantedText(wantedText) { start(); }

    int answered() const { return m_answered; }
    int missed() const { return m_missed; }
    QStringList texts() const { return m_texts; }

private:
    void start()
    {
        QObject::connect(&m_timer, &QTimer::timeout, [this]() { answer(); });
        m_timer.start(20);
    }

    QAbstractButton *wantedButton(QMessageBox *box) const
    {
        if (m_wantedText.isEmpty()) return box->button(m_wanted);
        for (QAbstractButton *b : box->buttons()) {
            if (b->text().remove(QLatin1Char('&')).contains(m_wantedText, Qt::CaseInsensitive))
                return b;
        }
        return nullptr;
    }

    void answer()
    {
        auto *box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        if (!box || box->property("answered").toBool()) return;
        box->setProperty("answered", true);
        m_texts << box->text();
        QAbstractButton *button = wantedButton(box);
        if (button) {
            ++m_answered;
        } else {
            ++m_missed;
            button = box->escapeButton() ? box->escapeButton() : box->buttons().value(0);
        }
        if (button) button->click();
    }

    QTimer m_timer;
    QMessageBox::StandardButton m_wanted = QMessageBox::NoButton;
    QString m_wantedText;
    int m_answered = 0;
    int m_missed = 0;
    QStringList m_texts;
};

// Picks one item in the next QInputDialog that opens, then accepts it.
class InputPicker
{
public:
    explicit InputPicker(const QString &choice)
    {
        QObject::connect(&m_timer, &QTimer::timeout, [this, choice]() {
            auto *dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget());
            if (!dialog) return;
            m_items = dialog->comboBoxItems();
            dialog->setTextValue(choice);
            ++m_shown;
            dialog->accept();
        });
        m_timer.start(20);
    }
    int shown() const { return m_shown; }
    QStringList items() const { return m_items; }

private:
    QTimer m_timer;
    int m_shown = 0;
    QStringList m_items;
};

bool showActive(QWidget &w)
{
    w.show();
    w.activateWindow();
    return QTest::qWaitForWindowActive(&w);
}

QString plain(const QAction *a) { return a->text().remove(QLatin1Char('&')); }

QMenu *encodingMenu(MainWindow &mw)
{
    for (QAction *a : mw.menuBar()->actions()) {
        if (plain(a) == QStringLiteral("Encoding")) return a->menu();
    }
    return nullptr;
}

QAction *menuItem(QMenu *menu, const QString &label)
{
    if (!menu) return nullptr;
    for (QAction *a : menu->actions()) {
        if (plain(a).startsWith(label)) return a;
    }
    return nullptr;
}

void typeAtEnd(Editor *editor, const QString &text)
{
    editor->setFocus();
    editor->moveCursor(QTextCursor::End);
    QTest::keyClicks(editor, text);
}

NotificationBar *bar(MainWindow &mw) { return mw.findChild<NotificationBar*>(); }

QString notificationText(MainWindow &mw)
{
    NotificationBar *b = bar(mw);
    if (!b || !b->isVisible()) return QString();
    QStringList parts;
    for (QLabel *label : b->findChildren<QLabel*>()) parts << label->text();
    return parts.join(QLatin1Char(' '));
}

QPushButton *visibleButton(QWidget *parent, const QString &label)
{
    for (QPushButton *b : parent->findChildren<QPushButton*>()) {
        if (b->isVisible() && b->text().remove(QLatin1Char('&')).startsWith(label)) return b;
    }
    return nullptr;
}

QString statusBarEncoding(MainWindow &mw, const QString &expected)
{
    auto *status = mw.findChild<StatusBarWidget*>();
    if (!status) return QStringLiteral("<no status bar>");
    for (QPushButton *b : status->findChildren<QPushButton*>()) {
        if (b->text() == expected) return b->text();
    }
    return QStringLiteral("<no button showing %1>").arg(expected);
}

QString path(const regression::TempDir &tmp, const char *name) { return tmp.filePath(QString::fromLatin1(name)); }

} // namespace

class TestT33EncodingUi : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();
    void ucs2ItemsWriteABom();
    void convertToAnsiWithArabicPromptsAndWritesNothingOnCancel();
    void convertReplaceIsOneUndoableEditThatSaves();
    void lossyDecodedDocumentRefusesKeystrokesAndOffersReopen();
    void reopenWithEncodingFromMenuRecoversWindows1252();
    void reopenWithEncodingRecoversMisdetectedWindows1256();
    void saveTimeEncodeLossyOffersSaveAsUtf8();
    void mixedEndingNoticeShownOncePerDocument();
    void replaceAllKeepsNoBreakSpaceAndLineSeparator();
    void encodingMenuAndStatusBarUseTheTableNames();
};

void TestT33EncodingUi::cleanup()
{
    DocumentManager::instance().closeAllDocuments();
}

// D6: the UCS-2 BOM items map to UTF-16BE-BOM and UTF-16LE-BOM and write a BOM.
void TestT33EncodingUi::ucs2ItemsWriteABom()
{
    regression::TempDir tmp;
    const QString file = path(tmp, "plain.txt");
    QVERIFY(regression::writeBytes(file, "hi\n"));
    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.openFile(file);
    QVERIFY(editor);

    mw.convertToUCS2LE();
    QCOMPARE(editor->document()->encoding(), QStringLiteral("UTF-16LE-BOM"));
    mw.saveFile();
    QCOMPARE(regression::readBytes(file), QByteArray("\xFF\xFEh\0i\0\n\0", 8));

    mw.convertToUCS2BE();
    QCOMPARE(editor->document()->encoding(), QStringLiteral("UTF-16BE-BOM"));
    mw.saveFile();
    QCOMPARE(regression::readBytes(file), QByteArray("\xFE\xFF\0h\0i\0\n", 8));
}

// D6: Convert to ANSI runs the encode first. Arabic cannot be written in
// ISO-8859-1, so the prompt names the count and positions; Cancel changes nothing.
void TestT33EncodingUi::convertToAnsiWithArabicPromptsAndWritesNothingOnCancel()
{
    regression::TempDir tmp;
    const QString file = path(tmp, "arabic.txt");
    const QByteArray bytes = QStringLiteral("abc مرحبا\n").toUtf8();
    QVERIFY(regression::writeBytes(file, bytes));
    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.openFile(file);
    QVERIFY(editor);
    const QString encodingBefore = editor->document()->encoding();

    PromptAnswerer cancel(QMessageBox::Cancel);
    mw.convertToANSI();
    QCOMPARE(cancel.answered(), 1);
    const QString prompt = cancel.texts().value(0);
    QVERIFY2(prompt.contains(QStringLiteral("5 characters")), qPrintable(prompt));
    QVERIFY2(prompt.contains(QStringLiteral("1:5")) && prompt.contains(QStringLiteral("1:9")), qPrintable(prompt));
    QCOMPARE(editor->document()->encoding(), encodingBefore);
    QVERIFY(!editor->document()->isModified());
    mw.saveFile();
    QCOMPARE(regression::readBytes(file), bytes);
}

// D6: "Replace N characters" is one undoable edit that puts ? at each
// unmappable position, so the converted file saves without loss.
void TestT33EncodingUi::convertReplaceIsOneUndoableEditThatSaves()
{
    regression::TempDir tmp;
    const QString file = path(tmp, "arabic.txt");
    QVERIFY(regression::writeBytes(file, QStringLiteral("abc مر\n").toUtf8()));
    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.openFile(file);
    QVERIFY(editor);

    {
        PromptAnswerer replace(QStringLiteral("Replace"));
        mw.convertToANSI();
        QCOMPARE(replace.missed(), 0);
        QCOMPARE(replace.answered(), 1);
    }
    QCOMPARE(editor->rawText(), QStringLiteral("abc ??\n"));
    QCOMPARE(editor->document()->encoding(), QStringLiteral("ISO-8859-1"));
    mw.saveFile();
    QCOMPARE(regression::readBytes(file), QByteArray("abc ??\n"));
    editor->undo();
    QCOMPARE(editor->rawText(), QStringLiteral("abc مر\n"));
}

// D6/D12: a lossy decode opens read-only, refuses keystrokes, and the
// notification offers Reopen with Encoding.
void TestT33EncodingUi::lossyDecodedDocumentRefusesKeystrokesAndOffersReopen()
{
    regression::TempDir tmp;
    const QString file = path(tmp, "cp932.txt");
    // 0xED 0x40 is a CP932 duplicate: it decodes, but re-encodes differently.
    const QByteArray bytes("abc \xED\x40 def\n");
    QVERIFY(regression::writeBytes(file, bytes));
    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.openFile(file);
    QVERIFY(editor);
    QVERIFY(editor->document()->wasDecodedLossily());

    const QString before = editor->rawText();
    typeAtEnd(editor, QStringLiteral("xyz"));
    QCOMPARE(editor->rawText(), before);
    QVERIFY(!editor->document()->isModified());

    QVERIFY2(notificationText(mw).contains(QStringLiteral("read-only")), qPrintable(notificationText(mw)));
    QPushButton *reopen = visibleButton(bar(mw), QStringLiteral("Reopen with Encoding"));
    QVERIFY2(reopen, "the lossy-load notification has no Reopen with Encoding button");
    InputPicker pick(QStringLiteral("Shift_JIS"));
    reopen->click();
    QCOMPARE(pick.shown(), 1);
    QCOMPARE(regression::readBytes(file), bytes);
}

// D6: Reopen with Encoding from the Encoding menu re-decodes the original bytes.
void TestT33EncodingUi::reopenWithEncodingFromMenuRecoversWindows1252()
{
    regression::TempDir tmp;
    const QString file = path(tmp, "cp1252.txt");
    const QByteArray bytes("\x93quoted\x94 \x80 5\n");
    QVERIFY(regression::writeBytes(file, bytes));
    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.openFile(file);
    QVERIFY(editor);

    QAction *reopen = menuItem(encodingMenu(mw), QStringLiteral("Reopen with Encoding"));
    QVERIFY2(reopen, "the Encoding menu has no Reopen with Encoding item");
    QVERIFY(reopen->isEnabled());
    InputPicker pick(QStringLiteral("windows-1252"));
    reopen->trigger();
    QCOMPARE(pick.shown(), 1);
    QVERIFY(pick.items().contains(QStringLiteral("windows-1252")));
    QCOMPARE(editor->document()->encoding(), QStringLiteral("windows-1252"));
    QCOMPARE(editor->rawText(), QStringLiteral("“quoted” € 5\n"));
    QVERIFY(!editor->isReadOnly());
    QVERIFY(!editor->document()->isModified());
    mw.saveFile();
    QCOMPARE(regression::readBytes(file), bytes);
}

// Detection does not recognise windows-1256; Reopen with Encoding corrects it.
void TestT33EncodingUi::reopenWithEncodingRecoversMisdetectedWindows1256()
{
    regression::TempDir tmp;
    const QString file = path(tmp, "cp1256.txt");
    const QByteArray bytes("\xE3\xD1\xCD\xC8\xC7\n");  // "marhaba" in windows-1256
    QVERIFY(regression::writeBytes(file, bytes));
    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.openFile(file);
    QVERIFY(editor);

    QAction *reopen = menuItem(encodingMenu(mw), QStringLiteral("Reopen with Encoding"));
    QVERIFY(reopen);
    InputPicker pick(QStringLiteral("windows-1256"));
    reopen->trigger();
    QCOMPARE(pick.shown(), 1);
    QCOMPARE(editor->rawText(), QStringLiteral("مرحبا\n"));
    QCOMPARE(statusBarEncoding(mw, QStringLiteral("windows-1256")), QStringLiteral("windows-1256"));
}

// D6: text typed that the file's encoding cannot hold gives EncodeLossy at
// save. The dialog offers Save as UTF-8, Replace or Cancel.
void TestT33EncodingUi::saveTimeEncodeLossyOffersSaveAsUtf8()
{
    regression::TempDir tmp;
    const QString file = path(tmp, "latin1.txt");
    const QByteArray bytes("plain caf\xE9\n");
    QVERIFY(regression::writeBytes(file, bytes));
    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.openFile(file);
    QVERIFY(editor);
    QCOMPARE(editor->document()->encoding(), QStringLiteral("ISO-8859-1"));
    editor->moveCursor(QTextCursor::End);
    editor->insertPlainText(QStringLiteral("م"));

    {
        PromptAnswerer cancel(QMessageBox::Cancel);
        mw.saveFile();
        QCOMPARE(cancel.answered(), 1);
        QVERIFY2(cancel.texts().value(0).contains(QStringLiteral("1 character")), qPrintable(cancel.texts().value(0)));
    }
    QCOMPARE(regression::readBytes(file), bytes);
    QVERIFY(editor->document()->isModified());

    PromptAnswerer utf8(QStringLiteral("Save as UTF-8"));
    mw.saveFile();
    QCOMPARE(utf8.missed(), 0);
    QCOMPARE(regression::readBytes(file), QStringLiteral("plain café\nم").toUtf8());
    QCOMPARE(editor->document()->encoding(), QStringLiteral("UTF-8"));
    QVERIFY(!editor->document()->isModified());
}

// D11: a file with mixed line endings is normalised at save, and the
// notification bar says so once per document.
void TestT33EncodingUi::mixedEndingNoticeShownOncePerDocument()
{
    regression::TempDir tmp;
    const QString file = path(tmp, "mixed.txt");
    QVERIFY(regression::writeBytes(file, "a\r\nb\r\nc\n"));
    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.openFile(file);
    QVERIFY(editor);
    QVERIFY(editor->document()->hadMixedLineEndings());

    typeAtEnd(editor, QStringLiteral("d"));
    mw.saveFile();
    QVERIFY2(notificationText(mw).contains(QStringLiteral("Mixed line endings")), qPrintable(notificationText(mw)));
    QVERIFY(notificationText(mw).contains(QStringLiteral("CRLF")));
    QCOMPARE(regression::readBytes(file), QByteArray("a\r\nb\r\nc\r\nd"));

    bar(mw)->hide();
    typeAtEnd(editor, QStringLiteral("e"));
    mw.saveFile();
    QVERIFY2(!notificationText(mw).contains(QStringLiteral("Mixed line endings")), "the notice was shown twice");
}

// D11: Replace All built its result from toPlainText(), which turned U+00A0
// into a space and U+2028 into a line feed. It must use rawText().
void TestT33EncodingUi::replaceAllKeepsNoBreakSpaceAndLineSeparator()
{
    regression::TempDir tmp;
    const QString file = path(tmp, "nbsp.txt");
    const QString text = QStringLiteral("one two foo three\n");
    QVERIFY(regression::writeBytes(file, text.toUtf8()));
    MainWindow mw;
    QVERIFY(showActive(mw));
    Editor *editor = mw.openFile(file);
    QVERIFY(editor);

    QVERIFY(QMetaObject::invokeMethod(&mw, "replace", Qt::DirectConnection));
    auto *dialog = mw.findChild<SearchDialog*>();
    QVERIFY(dialog);
    dialog->setSearchText(QStringLiteral("foo"));
    QComboBox *replaceWith = nullptr;
    for (QComboBox *combo : dialog->findChildren<QComboBox*>()) {
        if (combo->isVisible() && combo->isEditable() && combo->currentText().isEmpty()) replaceWith = combo;
    }
    QVERIFY(replaceWith);
    replaceWith->setCurrentText(QStringLiteral("bar"));
    QCOMPARE(dialog->replaceText(), QStringLiteral("bar"));
    emit dialog->replaceAll();
    dialog->hide();

    QCOMPARE(editor->rawText(), QStringLiteral("one two bar three\n"));
    mw.saveFile();
    QCOMPARE(regression::readBytes(file), QStringLiteral("one two bar three\n").toUtf8());
}

// W1 names encodings by their table (MIME/IANA) spelling. The Encoding menu's
// checked item and the status bar must follow those names, and Reopen with
// Encoding is the first Encoding-menu item and in the status-bar menu too.
void TestT33EncodingUi::encodingMenuAndStatusBarUseTheTableNames()
{
    regression::TempDir tmp;
    const QString utf16 = path(tmp, "utf16.txt");
    QVERIFY(regression::writeBytes(utf16, QByteArray("\xFF\xFEh\0i\0\n\0", 8)));
    const QString latin = path(tmp, "latin1.txt");
    QVERIFY(regression::writeBytes(latin, "plain caf\xE9\n"));
    MainWindow mw;
    QVERIFY(showActive(mw));
    QMenu *menu = encodingMenu(mw);
    QVERIFY(menu);
    QVERIFY2(plain(menu->actions().value(0)).startsWith(QStringLiteral("Reopen with Encoding")),
             qPrintable(plain(menu->actions().value(0))));

    Editor *e16 = mw.openFile(utf16);
    QVERIFY(e16);
    QCOMPARE(e16->document()->encoding(), QStringLiteral("UTF-16LE-BOM"));
    QVERIFY(menuItem(menu, QStringLiteral("Encode in UCS-2 LE BOM"))->isChecked());
    QVERIFY(!menuItem(menu, QStringLiteral("Encode in UTF-8"))->isChecked());
    QCOMPARE(statusBarEncoding(mw, QStringLiteral("UTF-16LE-BOM")), QStringLiteral("UTF-16LE-BOM"));

    Editor *eLatin = mw.openFile(latin);
    QVERIFY(eLatin);
    QCOMPARE(eLatin->document()->encoding(), QStringLiteral("ISO-8859-1"));
    QVERIFY(menuItem(menu, QStringLiteral("Encode in ANSI"))->isChecked());
    QVERIFY(!menuItem(menu, QStringLiteral("Encode in UCS-2 LE BOM"))->isChecked());

    auto *status = mw.findChild<StatusBarWidget*>();
    QVERIFY(status);
    bool statusOffersReopen = false;
    for (QMenu *m : status->findChildren<QMenu*>()) {
        if (menuItem(m, QStringLiteral("Reopen with Encoding"))) statusOffersReopen = true;
    }
    QVERIFY2(statusOffersReopen, "the status-bar encoding menu has no Reopen with Encoding item");
}

PRIMEEDIT_REGRESSION_MAIN(TestT33EncodingUi)

#include "test_t3_3_encoding_ui.moc"

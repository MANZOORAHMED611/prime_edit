// Notepad++ Edit > Insert, Edit > Copy to Clipboard, and the Tools menu
// hashes, MIME Tools and Converter.
#include "mainwindow.h"
#include "editor.h"
#include "core/codectransforms.h"
#include "core/document.h"
#include "core/texttransforms.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QLocale>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStatusBar>
#include <QVBoxLayout>

// Notepad++ ships these without default shortcuts.
void MainWindow::setupInsertAndClipboardMenus()
{
    QMenu *insertMenu = m_editMenu->addMenu(tr("Insert"));
    insertMenu->addAction(tr("Date Time (short)"), this, [this]() { insertDateTime(false); });
    insertMenu->addAction(tr("Date Time (long)"), this, [this]() { insertDateTime(true); });

    QMenu *copyMenu = m_editMenu->addMenu(tr("Copy to Clipboard"));
    copyMenu->addAction(tr("Current Full File Path to Clipboard"), this,
                        [this]() { copyPathToClipboard(PathPart::FullPath); });
    copyMenu->addAction(tr("Current Filename to Clipboard"), this,
                        [this]() { copyPathToClipboard(PathPart::FileName); });
    copyMenu->addAction(tr("Current Dir. Path to Clipboard"), this,
                        [this]() { copyPathToClipboard(PathPart::Directory); });
}

// Replaces the selection, or inserts at the cursor, as one undo step.
void MainWindow::insertDateTime(bool longFormat)
{
    Editor *editor = currentEditor();
    if (!editor)
        return;
    if (editor->isReadOnly()) {
        showNotification(tr("'%1' is read-only, so it was not changed.")
                             .arg(editor->document()->displayName()));
        return;
    }
    const QString stamp = TextTransforms::formatDateTime(QDateTime::currentDateTime(),
                                                         QLocale::system(), longFormat);
    const QTextCursor cursor = editor->textCursor();
    editor->replaceRange(cursor.selectionStart(), cursor.selectionEnd(), stamp, false);
}

// Tools > MD5 / SHA-1 / SHA-256 / SHA-512, MIME Tools and Converter. Notepad++
// keeps the last two in plugins; they have no default shortcuts.
void MainWindow::setupNppToolsMenu()
{
    using CodecTransforms::HashAlgorithm;
    const QList<QPair<QString, HashAlgorithm>> hashes = {
        {QStringLiteral("MD5"), HashAlgorithm::Md5}, {QStringLiteral("SHA-1"), HashAlgorithm::Sha1},
        {QStringLiteral("SHA-256"), HashAlgorithm::Sha256}, {QStringLiteral("SHA-512"), HashAlgorithm::Sha512}};
    m_toolsMenu->addSeparator();
    for (const auto &[name, algorithm] : hashes) {
        QMenu *menu = m_toolsMenu->addMenu(name);
        menu->addAction(tr("Generate from Selection or Document..."), this,
                        [this, name = name, algorithm = algorithm]() { showTextHash(name, algorithm); })
            ->setObjectName(QStringLiteral("hashText ") + name);
        menu->addAction(tr("Generate from Files..."), this,
                        [this, name = name, algorithm = algorithm]() { showFileHashes(name, algorithm); });
    }
    const auto codec = [this](QMenu *menu, const QString &text, const CodecFn &fn) {
        menu->addAction(text, this, [this, text, fn]() { applyCodec(text, fn); });
    };
    const auto always = [](QString (*fn)(const QString &)) {
        return [fn](const QString &t) { return std::optional<QString>(fn(t)); };
    };
    QMenu *mime = m_toolsMenu->addMenu(tr("MIME Tools"));
    codec(mime, tr("Base64 Encode"), always(CodecTransforms::base64Encode));
    codec(mime, tr("Base64 Decode"), CodecTransforms::base64Decode);
    codec(mime, tr("URL Encode"), always(CodecTransforms::urlEncode));
    codec(mime, tr("URL Decode"), CodecTransforms::urlDecode);
    QMenu *converter = m_toolsMenu->addMenu(tr("Converter"));
    codec(converter, tr("ASCII -> HEX"), always(CodecTransforms::asciiToHex));
    codec(converter, tr("HEX -> ASCII"), CodecTransforms::hexToAscii);
}

// Encoders and decoders work on the selection only. A decoder that rejects
// its input leaves the text as it was and says why; it never replaces it.
void MainWindow::applyCodec(const QString &command, const CodecFn &fn)
{
    Editor *editor = currentEditor();
    if (!editor)
        return;
    if (!editor->textCursor().hasSelection()) {
        showNotification(tr("%1: select the text to convert first.").arg(command));
        return;
    }
    bool valid = true;
    transformCurrent(TransformScope::Selection, [&fn, &valid](const QString &t) {
        const std::optional<QString> result = fn(t);
        valid = result.has_value();
        return result.value_or(t);
    });
    if (!valid)
        showNotification(tr("%1: the selection is not valid input, so it was not changed.").arg(command));
}

// The digest of the selection, or of the whole document when nothing is
// selected, as the editor holds it (LF line breaks, UTF-8).
void MainWindow::showTextHash(const QString &name, CodecTransforms::HashAlgorithm algorithm)
{
    Editor *editor = currentEditor();
    if (!editor)
        return;
    const QTextCursor cursor = editor->textCursor();
    const QString text = cursor.hasSelection()
                             ? editor->textInRange(cursor.selectionStart(), cursor.selectionEnd())
                             : editor->rawText();
    showHashResult(tr("%1 of %2").arg(name, cursor.hasSelection() ? tr("the selection")
                                                                  : editor->document()->displayName()),
                   CodecTransforms::hashText(text, algorithm));
}

void MainWindow::showFileHashes(const QString &name, CodecTransforms::HashAlgorithm algorithm)
{
    const QStringList files = QFileDialog::getOpenFileNames(this, tr("Generate %1 from Files").arg(name));
    if (files.isEmpty())
        return;
    QStringList lines;
    for (const QString &path : files) {
        const std::optional<QString> digest = CodecTransforms::hashFile(path, algorithm);
        lines << (digest ? *digest + QStringLiteral("  ") + path : tr("%1: could not be read").arg(path));
    }
    showHashResult(tr("%1 of %n file(s)", nullptr, int(files.size())).arg(name), lines.join(QLatin1Char('\n')));
}

// A read-only result with a Copy button, so nothing is written to the document.
void MainWindow::showHashResult(const QString &title, const QString &result)
{
    QDialog dialog(this);
    dialog.setWindowTitle(title);
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QPlainTextEdit *view = new QPlainTextEdit(result, &dialog);
    view->setReadOnly(true);
    view->setLineWrapMode(QPlainTextEdit::NoWrap);
    layout->addWidget(view);
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    QPushButton *copy = buttons->addButton(tr("Copy"), QDialogButtonBox::ActionRole);
    connect(copy, &QPushButton::clicked, &dialog, [result]() { QApplication::clipboard()->setText(result); });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    dialog.resize(640, 200);
    dialog.exec();
}

void MainWindow::copyPathToClipboard(PathPart part)
{
    Editor *editor = currentEditor();
    if (!editor)
        return;
    const Document *doc = editor->document();
    if (doc->isUntitled() && part != PathPart::FileName) {
        showNotification(tr("'%1' has not been saved, so it has no path.").arg(doc->displayName()));
        return;
    }
    const QFileInfo info(doc->filePath());
    QString text;
    switch (part) {
    case PathPart::FullPath: text = QDir::toNativeSeparators(info.absoluteFilePath()); break;
    case PathPart::FileName: text = doc->isUntitled() ? doc->displayName() : info.fileName(); break;
    case PathPart::Directory: text = QDir::toNativeSeparators(info.absolutePath()); break;
    }
    QApplication::clipboard()->setText(text);
    statusBar()->showMessage(tr("Copied %1").arg(text), 3000);
}

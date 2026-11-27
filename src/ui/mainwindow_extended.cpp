// Extended MainWindow implementations for Notepad++ style functionality
// This file contains stub implementations for all the toolbar functions

#include "mainwindow.h"
#include "editor.h"
#include "tabwidget.h"
#include "statusbar.h"
#include "evalresultwidget.h"
#include "endpointconfigdialog.h"
#include "gitcommitdialog.h"
#include "remoteconnectiondialog.h"
#include "notificationbar.h"
#include "core/remoteconnection.h"
#include "core/document.h"
#include "core/textcodec.h"
#include "core/documentmanager.h"
#include "core/llmevaluator.h"
#include "core/islamicbridge.h"
#include "core/schemavalidator.h"
#include "core/lspmanager.h"
#include "core/lspclient.h"
#include <QMessageBox>
#include <QTextBlock>
#include <QTextCursor>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QActionGroup>
#include <QPushButton>
#include <QInputDialog>
#include <QStandardPaths>
#include <QLineEdit>
#include <QToolTip>
#include "columneditor.h"

// Comment operations
void MainWindow::blockComment()
{
    Editor *editor = currentEditor();
    if (!editor) return;

    QTextCursor cursor = editor->textCursor();
    QString selectedText = cursor.selectedText();

    // Add block comment markers (/* */)
    cursor.insertText("/* " + selectedText + " */");
}

void MainWindow::streamComment()
{
    Editor *editor = currentEditor();
    if (!editor) return;

    QTextCursor cursor = editor->textCursor();
    QString selectedText = cursor.selectedText();

    // Stream comment - add // to each line
    QStringList lines = selectedText.split(QChar::ParagraphSeparator);
    for (QString &line : lines) {
        line = "// " + line;
    }
    cursor.insertText(lines.join("\n"));
}

// Line operations
void MainWindow::removeEmptyLines()
{
    Editor *editor = currentEditor();
    if (!editor) return;

    QString text = editor->rawText();
    QStringList lines = text.split('\n');
    QStringList nonEmptyLines;

    for (const QString &line : lines) {
        if (!line.trimmed().isEmpty()) {
            nonEmptyLines.append(line);
        }
    }

    QString newText = nonEmptyLines.join('\n');
    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(newText);
    cursor.endEditBlock();
}

void MainWindow::removeEmptyLinesWithBlanks()
{
    Editor *editor = currentEditor();
    if (!editor) return;

    QString text = editor->rawText();
    QStringList lines = text.split('\n');
    QStringList filteredLines;

    for (const QString &line : lines) {
        bool hasNonBlank = false;
        for (const QChar &ch : line) {
            if (!ch.isSpace()) {
                hasNonBlank = true;
                break;
            }
        }
        if (hasNonBlank) {
            filteredLines.append(line);
        }
    }

    QString newText = filteredLines.join('\n');
    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(newText);
    cursor.endEditBlock();
}

void MainWindow::removeConsecutiveDuplicateLines()
{
    Editor *editor = currentEditor();
    if (!editor) return;

    QString text = editor->rawText();
    QStringList lines = text.split('\n');
    QStringList uniqueLines;

    QString previousLine;
    for (const QString &line : lines) {
        if (line != previousLine || uniqueLines.isEmpty()) {
            uniqueLines.append(line);
        }
        previousLine = line;
    }

    QString newText = uniqueLines.join('\n');
    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(newText);
    cursor.endEditBlock();
}

void MainWindow::splitLines()
{
    Editor *editor = currentEditor();
    if (!editor) return;

    QTextCursor cursor = editor->textCursor();
    cursor.insertText("\n");
}

void MainWindow::trimLeadingWhitespace()
{
    Editor *editor = currentEditor();
    if (!editor) return;

    QString text = editor->rawText();
    QStringList lines = text.split('\n');

    for (QString &line : lines) {
        int i = 0;
        while (i < line.length() && line[i].isSpace()) {
            i++;
        }
        line = line.mid(i);
    }

    QString newText = lines.join('\n');
    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(newText);
    cursor.endEditBlock();
}

void MainWindow::trimLeadingAndTrailing()
{
    Editor *editor = currentEditor();
    if (!editor) return;

    QString text = editor->rawText();
    QStringList lines = text.split('\n');

    for (QString &line : lines) {
        line = line.trimmed();
    }

    QString newText = lines.join('\n');
    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(newText);
    cursor.endEditBlock();
}

void MainWindow::eolToSpace()
{
    Editor *editor = currentEditor();
    if (!editor) return;

    QString text = editor->rawText();
    text.replace('\n', ' ');
    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(text);
    cursor.endEditBlock();
}

void MainWindow::removeUnnecessaryBlanks()
{
    Editor *editor = currentEditor();
    if (!editor) return;

    QString text = editor->rawText();

    // Remove multiple consecutive blank lines
    text.replace(QRegularExpression("\n\\s*\n\\s*\n"), "\n\n");

    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(text);
    cursor.endEditBlock();
}

void MainWindow::tabToSpace()
{
    Editor *editor = currentEditor();
    if (!editor) return;

    QString text = editor->rawText();
    text.replace('\t', "    "); // 4 spaces per tab
    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(text);
    cursor.endEditBlock();
}

void MainWindow::spaceToTabAll()
{
    Editor *editor = currentEditor();
    if (!editor) return;

    QString text = editor->rawText();
    text.replace("    ", "\t"); // 4 spaces to tab
    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(text);
    cursor.endEditBlock();
}

void MainWindow::spaceToTabLeading()
{
    Editor *editor = currentEditor();
    if (!editor) return;

    QString text = editor->rawText();
    QStringList lines = text.split('\n');

    for (QString &line : lines) {
        // Only convert leading spaces
        int spaceCount = 0;
        while (spaceCount < line.length() && line[spaceCount] == ' ') {
            spaceCount++;
        }

        int tabs = spaceCount / 4;
        int remaining = spaceCount % 4;

        line = QString("\t").repeated(tabs) + QString(" ").repeated(remaining) + line.mid(spaceCount);
    }

    QString newText = lines.join('\n');
    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(newText);
    cursor.endEditBlock();
}

// Text case operations
void MainWindow::toSentenceCase()
{
    Editor *editor = currentEditor();
    if (!editor) return;

    QTextCursor cursor = editor->textCursor();
    QString selectedText = cursor.selectedText();

    if (selectedText.isEmpty()) return;

    QString result = selectedText.toLower();
    if (!result.isEmpty()) {
        result[0] = result[0].toUpper();
    }

    cursor.insertText(result);
}

void MainWindow::toInvertCase()
{
    Editor *editor = currentEditor();
    if (!editor) return;

    QTextCursor cursor = editor->textCursor();
    QString selectedText = cursor.selectedText();

    QString result;
    for (const QChar &ch : selectedText) {
        if (ch.isUpper()) {
            result += ch.toLower();
        } else if (ch.isLower()) {
            result += ch.toUpper();
        } else {
            result += ch;
        }
    }

    cursor.insertText(result);
}

void MainWindow::toRandomCase()
{
    Editor *editor = currentEditor();
    if (!editor) return;

    QTextCursor cursor = editor->textCursor();
    QString selectedText = cursor.selectedText();

    QString result;
    for (const QChar &ch : selectedText) {
        if (ch.isLetter()) {
            bool upper = QRandomGenerator::global()->bounded(2) == 1;
            result += upper ? ch.toUpper() : ch.toLower();
        } else {
            result += ch;
        }
    }

    cursor.insertText(result);
}

// View operations
void MainWindow::toggleWhitespace()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->setShowWhitespace(!editor->showWhitespace());
    }
}

void MainWindow::toggleEndOfLine()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->setShowEOL(!editor->showEOL());
    }
}

void MainWindow::toggleIndentGuide()
{
    Editor *editor = currentEditor();
    if (editor) {
        editor->setShowIndentGuide(!editor->showIndentGuide());
    }
}

void MainWindow::toggleWrapSymbol()
{
    // TODO: Implement wrap symbol toggle
    statusBar()->showMessage(tr("Toggle wrap symbol not yet implemented"), 3000);
}

void MainWindow::toggleFullScreen()
{
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}

void MainWindow::toggleDistractionFree()
{
    // Hide/show all toolbars, menus, status bar
    m_distractionFree = !m_distractionFree;

    menuBar()->setVisible(!m_distractionFree);
    statusBar()->setVisible(!m_distractionFree);

    // Hide all toolbars
    for (QToolBar *toolbar : findChildren<QToolBar*>()) {
        toolbar->setVisible(!m_distractionFree);
    }
}

void MainWindow::syncVerticalScroll()
{
    // TODO: Implement vertical scroll sync
    statusBar()->showMessage(tr("Sync vertical scroll not yet implemented"), 3000);
}

void MainWindow::syncHorizontalScroll()
{
    // TODO: Implement horizontal scroll sync
    statusBar()->showMessage(tr("Sync horizontal scroll not yet implemented"), 3000);
}

// The editor a conversion applies to, or null. A partial view holds only part
// of the file, so its encoding and line endings cannot change (D3).
Editor *MainWindow::convertibleEditor()
{
    Editor *e = currentEditor();
    if (!e || !e->document()) return nullptr;
    if (e->document()->isPartialView()) {
        showNotification(tr("Only part of '%1' is loaded, so its encoding and line endings "
                            "cannot be changed.").arg(e->document()->displayName()));
        return nullptr;
    }
    if (!isConvertible(e->document())) {
        showNotification(tr("'%1' is read-only, so its encoding and line endings cannot be "
                            "changed. Use Reopen with Encoding to read it differently.")
                             .arg(e->document()->displayName()));
        return nullptr;
    }
    return e;
}

// A conversion only marks the document for a rewrite, so it is offered only
// when that rewrite can happen: not for a read-only document (lossy decode,
// partial view) nor for a file without write permission.
bool MainWindow::isConvertible(const Document *doc) const
{
    if (!doc || doc->isPartialView() || doc->isReadOnly()) return false;
    return doc->isUntitled() || QFileInfo(doc->filePath()).isWritable();
}

// ---- Reload bar (spec D8) ----

void MainWindow::onFileExternallyModified(const QString &path)
{
    Document *doc = DocumentManager::instance().findDocument(path);
    if (!doc) return;
    m_pendingReloads.insert(path);
    Editor *e = currentEditor();
    if (e && e->document() == doc) {
        showReloadBar(doc);  // a background tab gets the bar when activated
    }
}

void MainWindow::showPendingReloadBar()
{
    Editor *e = currentEditor();
    if (e && e->document() && m_pendingReloads.contains(e->document()->filePath())) {
        showReloadBar(e->document());
    }
}

void MainWindow::showReloadBar(Document *doc)
{
    const bool edited = doc->isModified();
    m_notificationBar->setProperty("reloadPath", doc->filePath());
    m_notificationBar->showChoice(
        edited ? tr("'%1' was changed by another program, and you have unsaved changes here.")
                     .arg(doc->displayName())
               : tr("'%1' was changed by another program.").arg(doc->displayName()),
        edited ? tr("Reload (discard my changes)") : tr("Reload"), tr("Keep mine"));
}

void MainWindow::onReloadAccepted()
{
    const QString path = m_notificationBar->property("reloadPath").toString();
    m_pendingReloads.remove(path);
    Document *doc = DocumentManager::instance().findDocument(path);
    if (!doc) return;
    for (Editor *e : allEditors()) {
        if (e->document() == doc) {
            reloadEditor(e);
            return;
        }
    }
}

void MainWindow::onReloadRejected()
{
    // "Keep mine": the edits stay, and the next external change asks again
    m_pendingReloads.remove(m_notificationBar->property("reloadPath").toString());
}

// Reloads the document from disk and refills its editor clean, so undo cannot
// make the reloaded text look modified (D1 reload through runSyncedEdit).
bool MainWindow::reloadEditor(Editor *editor)
{
    Document *doc = editor->document();
    if (!doc->load(doc->filePath())) {
        QMessageBox::warning(this, tr("Reload"), tr("Could not reload '%1'.").arg(doc->displayName()));
        return false;
    }
    editor->runSyncedEdit(SyncTarget::Clean, [editor]() { editor->syncFromDocument(); });
    m_pendingReloads.remove(doc->filePath());
    updateTabTextForDocument(doc);
    updateWindowTitle();
    updateStatusBar();
    updateMenuState();
    return true;
}

bool MainWindow::anySaveableModified() const
{
    for (const Editor *e : allEditors()) {
        const Document *doc = e->document();
        if (doc && doc->isModified() && !doc->isPartialView()) return true;
    }
    return false;
}

QString MainWindow::partialViewNotice(const Document *doc) const
{
    const qint64 mib = QFileInfo(doc->filePath()).size() / (1024 * 1024);
    return tr("'%1' is large (%2 MB), so only part of it is loaded. It is read-only: "
              "saving and encoding or line-ending changes are disabled.")
        .arg(doc->displayName()).arg(mib);
}

// Encoding operations
void MainWindow::convertToANSI()
{
    convertEncoding(QStringLiteral("ISO-8859-1"), tr("ANSI (ISO-8859-1)"));
}

void MainWindow::convertToUTF8()
{
    convertEncoding(QStringLiteral("UTF-8"), QStringLiteral("UTF-8"));
}

void MainWindow::convertToUTF8BOM()
{
    convertEncoding(QStringLiteral("UTF-8-BOM"), QStringLiteral("UTF-8-BOM"));
}

void MainWindow::convertToUCS2BE()
{
    convertEncoding(QStringLiteral("UTF-16BE-BOM"), tr("UCS-2 Big Endian with BOM (UTF-16BE-BOM)"));
}

void MainWindow::convertToUCS2LE()
{
    convertEncoding(QStringLiteral("UTF-16LE-BOM"), tr("UCS-2 Little Endian with BOM (UTF-16LE-BOM)"));
}

// ============================================================
// Encoding tools (spec D6, D11): Reopen with Encoding, the convert preflight,
// the save-time EncodeLossy dialog and the mixed-line-ending notice.
// ============================================================

namespace {

// The encodings Reopen with Encoding offers. Each goes through
// TextCodec::canonicalName, so the list uses the table spelling of spec 9.1.
const char *const kReopenEncodings[] = {
    "UTF-8", "UTF-8-BOM", "UTF-16LE", "UTF-16BE", "UTF-16LE-BOM", "UTF-16BE-BOM",
    "UTF-32LE", "UTF-32BE", "windows-1252", "ISO-8859-1", "ISO-8859-6",
    "windows-1256", "Shift_JIS",
};

QString characterCount(int n)
{
    return n == 1 ? QObject::tr("1 character") : QObject::tr("%1 characters").arg(n);
}

QString lineEndingName(Document::LineEnding ending)
{
    switch (ending) {
    case Document::Windows: return QStringLiteral("CRLF");
    case Document::ClassicMac: return QStringLiteral("CR");
    case Document::Unix: break;
    }
    return QStringLiteral("LF");
}

} // namespace

// Adds Reopen with Encoding as the first Encoding-menu item and makes the
// "Encode in ..." items track the current document (setupMenus gave each its
// encoding name as data).
void MainWindow::setupEncodingTools()
{
    QAction *first = m_encodingMenu->actions().value(0);
    m_reopenEncodingAction = new QAction(tr("Reopen with Encoding..."), this);
    m_reopenEncodingAction->setToolTip(tr("Decode the file's bytes again with an encoding you choose"));
    connect(m_reopenEncodingAction, &QAction::triggered, this, &MainWindow::reopenWithEncoding);
    m_encodingMenu->insertAction(first, m_reopenEncodingAction);
    m_encodingMenu->insertSeparator(first);
    for (QAction *action : m_encodingMenu->actions()) {
        if (action->data().isNull()) continue;
        action->setData(TextCodec::canonicalName(action->data().toString()));
        action->setCheckable(true);
        // Optional: a windows-1252 or Shift_JIS file has no matching item
        if (action->actionGroup())
            action->actionGroup()->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
    }
    // The lossy-load notification's button reopens the document it named
    connect(m_notificationBar, &NotificationBar::actionTriggered, this, [this]() {
        const QString path = m_notificationBar->property("reopenPath").toString();
        if (!path.isEmpty() && activateOpenEditor(path)) reopenWithEncoding();
    });
}

// Checks the item whose encoding name equals the current document's, and
// enables the conversions only when the document can be converted.
void MainWindow::syncEncodingMenu(bool convertible)
{
    if (!m_encodingMenu) return;
    Editor *e = currentEditor();
    const Document *doc = e ? e->document() : nullptr;
    const QString current = doc ? doc->encoding() : QString();
    for (QAction *action : m_encodingMenu->actions()) {
        if (!action->isCheckable()) continue;
        action->setEnabled(convertible);
        action->setChecked(action->data().toString() == current);
    }
    if (m_reopenEncodingAction)
        m_reopenEncodingAction->setEnabled(doc && !doc->isUntitled() && !doc->isPartialView());
}

QStringList MainWindow::reopenEncodingNames() const
{
    QStringList names;
    for (const char *name : kReopenEncodings) {
        const QString canonical = TextCodec::canonicalName(QString::fromLatin1(name));
        if (!canonical.isEmpty() && !names.contains(canonical)) names << canonical;
    }
    return names;
}

void MainWindow::reopenWithEncoding()
{
    Editor *e = currentEditor();
    Document *doc = e ? e->document() : nullptr;
    if (!doc || doc->isUntitled() || doc->isPartialView()) {
        showNotification(tr("Reopen with Encoding needs a saved file that is fully loaded."));
        return;
    }
    if (doc->isModified()
        && QMessageBox::question(this, tr("Reopen with Encoding"),
                                 tr("Reopening '%1' discards your unsaved changes. Continue?")
                                     .arg(doc->displayName()),
                                 QMessageBox::Discard | QMessageBox::Cancel) != QMessageBox::Discard) {
        return;
    }
    const QStringList names = reopenEncodingNames();
    bool ok = false;
    const QString name = QInputDialog::getItem(
        this, tr("Reopen with Encoding"), tr("Decode '%1' as:").arg(doc->displayName()),
        names, qMax(0, names.indexOf(doc->encoding())), false, &ok);
    if (ok) reopenEditorWithEncoding(e, name);
}

// A reload, not a conversion: the file's bytes are decoded again and the
// editor is refilled clean (D8 reload contract: runSyncedEdit(Clean)).
void MainWindow::reopenEditorWithEncoding(Editor *editor, const QString &name)
{
    Document *doc = editor->document();
    if (!doc->reloadWithEncoding(name)) {
        QMessageBox::warning(this, tr("Reopen with Encoding"),
                             tr("Could not reopen '%1' as %2.").arg(doc->displayName(), name));
        return;
    }
    editor->runSyncedEdit(SyncTarget::Clean, [editor]() { editor->syncFromDocument(); });
    m_statusBar->updateFromEditor(editor);
    updateMenuState();
    updateWindowTitle();
    if (doc->wasDecodedLossily()) {
        showReopenNotice(doc);
    } else {
        showNotification(tr("Reopened '%1' as %2.").arg(doc->displayName(), doc->encoding()));
    }
}

void MainWindow::showOpenNotices(Document *doc)
{
    if (doc->isPartialView()) {
        showNotification(partialViewNotice(doc));
    } else if (doc->wasDecodedLossily()) {
        showReopenNotice(doc);
    }
}

void MainWindow::showReopenNotice(Document *doc)
{
    m_notificationBar->setProperty("reopenPath", doc->filePath());
    m_notificationBar->showAction(
        tr("'%1' does not decode exactly as %2, so it is read-only to protect the file. "
           "If the text looks wrong, reopen it with the right encoding.")
            .arg(doc->displayName(), doc->encoding()),
        tr("Reopen with Encoding..."));
}

// Asks what to do with characters the target encoding cannot hold. Names the
// count and the first 5 positions as line:column (spec D6).
MainWindow::UnmappableChoice MainWindow::askUnmappable(Editor *editor, const QString &encoding,
                                                       const QList<int> &positions, bool offerUtf8)
{
    const QTextDocument *text = static_cast<QPlainTextEdit*>(editor)->document();
    QStringList where;
    for (int i = 0; i < qMin(5, int(positions.size())); ++i) {
        const QTextBlock block = text->findBlock(positions[i]);
        where << QStringLiteral("%1:%2").arg(block.blockNumber() + 1).arg(positions[i] - block.position() + 1);
    }
    QMessageBox box(QMessageBox::Warning, tr("Characters Cannot Be Encoded"),
                    tr("%1 cannot be written in %2. First positions (line:column): %3.")
                        .arg(characterCount(int(positions.size())), encoding, where.join(QStringLiteral(", "))),
                    QMessageBox::Cancel, this);
    QPushButton *replace = box.addButton(
        tr("Replace %1 with ?").arg(characterCount(int(positions.size()))), QMessageBox::DestructiveRole);
    QPushButton *utf8 = offerUtf8 ? box.addButton(tr("Save as UTF-8"), QMessageBox::AcceptRole) : nullptr;
    box.setDefaultButton(QMessageBox::Cancel);
    box.exec();
    if (box.clickedButton() == replace) return UnmappableChoice::Replace;
    if (utf8 && box.clickedButton() == utf8) return UnmappableChoice::SaveUtf8;
    return UnmappableChoice::Cancel;
}

// One undoable edit that puts ? at each unmappable position, so the user sees
// exactly what will be written. A surrogate pair becomes a single ?.
void MainWindow::replaceUnmappable(Editor *editor, const QList<int> &positions)
{
    QTextDocument *text = static_cast<QPlainTextEdit*>(editor)->document();
    QTextCursor cursor(text);
    cursor.beginEditBlock();
    for (auto it = positions.crbegin(); it != positions.crend(); ++it) {
        const int pos = *it;
        const QChar ch = text->characterAt(pos);
        if (ch.isLowSurrogate() && text->characterAt(pos - 1).isHighSurrogate()) continue;
        const int length = (ch.isHighSurrogate() && text->characterAt(pos + 1).isLowSurrogate()) ? 2 : 1;
        cursor.setPosition(pos);
        cursor.setPosition(pos + length, QTextCursor::KeepAnchor);
        cursor.insertText(QStringLiteral("?"));
    }
    cursor.endEditBlock();
}

// Convert preflight: encode first, and change the encoding only when nothing
// would be lost or the user chose to replace the unmappable characters.
void MainWindow::convertEncoding(const QString &name, const QString &label)
{
    Editor *e = convertibleEditor();
    if (!e) return;
    const QString encoding = TextCodec::canonicalName(name);
    const EncodeResult encoded = TextCodec::encode(e->rawText(), encoding);
    if (!encoded.unmappable.isEmpty()) {
        if (askUnmappable(e, encoding, encoded.unmappable, false) != UnmappableChoice::Replace) {
            updateMenuState();  // the click checked the item; put the check back
            return;
        }
        replaceUnmappable(e, encoded.unmappable);
    }
    e->document()->setEncoding(encoding);
    m_statusBar->updateFromEditor(e);
    updateMenuState();
    statusBar()->showMessage(tr("Encoding changed to %1").arg(label), 3000);
}

// Save-time EncodeLossy (spec D6): Save as UTF-8, Replace N characters, or
// Cancel. The file is untouched until one of the first two succeeds.
bool MainWindow::resolveEncodeLossy(Editor *editor, const QString &path)
{
    Document *doc = editor->document();
    const EncodeResult encoded = TextCodec::encode(editor->rawText(), doc->encoding());
    if (encoded.unmappable.isEmpty()) {
        reportSaveFailure(doc, SaveResult::EncodeLossy);
        return false;
    }
    const UnmappableChoice choice = askUnmappable(editor, doc->encoding(), encoded.unmappable, true);
    if (choice == UnmappableChoice::Cancel) return false;
    if (choice == UnmappableChoice::SaveUtf8) {
        doc->setEncoding(TextCodec::canonicalName(QStringLiteral("UTF-8")));
    } else {
        replaceUnmappable(editor, encoded.unmappable);
    }
    editor->syncToDocument();
    const SaveResult retry = doc->saveAs(path);
    if (retry != SaveResult::Ok) {
        reportSaveFailure(doc, retry);
        return false;
    }
    return true;
}

// D11: a file loaded with mixed line endings is saved with its majority
// ending. Say so once per document, at its first save.
void MainWindow::noteMixedLineEndings(Document *doc)
{
    if (!doc->hadMixedLineEndings() || doc->property("mixedEndingNoticeShown").toBool()) return;
    doc->setProperty("mixedEndingNoticeShown", true);
    showNotification(tr("Mixed line endings in '%1' will be saved as %2.")
                         .arg(doc->displayName(), lineEndingName(doc->lineEnding())));
}

// Line ending operations
void MainWindow::convertToWindows()
{
    Editor *e = convertibleEditor();
    if (!e) return;
    e->document()->setLineEnding(Document::Windows);
    m_statusBar->updateFromEditor(e);
    statusBar()->showMessage(tr("Line ending changed to Windows (CRLF)"), 3000);
}

void MainWindow::convertToUnix()
{
    Editor *e = convertibleEditor();
    if (!e) return;
    e->document()->setLineEnding(Document::Unix);
    m_statusBar->updateFromEditor(e);
    statusBar()->showMessage(tr("Line ending changed to Unix (LF)"), 3000);
}

void MainWindow::convertToMac()
{
    Editor *e = convertibleEditor();
    if (!e) return;
    e->document()->setLineEnding(Document::ClassicMac);
    m_statusBar->updateFromEditor(e);
    statusBar()->showMessage(tr("Line ending changed to Mac (CR)"), 3000);
}

// Bookmark operations
void MainWindow::toggleBookmark()
{
    Editor *e = currentEditor();
    if (e) e->toggleBookmark(e->currentLine());
}

void MainWindow::nextBookmark()
{
    Editor *e = currentEditor();
    if (e) e->nextBookmark();
}

void MainWindow::previousBookmark()
{
    Editor *e = currentEditor();
    if (e) e->previousBookmark();
}

void MainWindow::clearAllBookmarks()
{
    Editor *e = currentEditor();
    if (e) e->clearBookmarks();
}

// Folding operations
void MainWindow::foldAll()
{
    Editor *e = currentEditor();
    if (e) e->foldAll();
}

void MainWindow::unfoldAll()
{
    Editor *e = currentEditor();
    if (e) e->unfoldAll();
}

void MainWindow::toggleFold()
{
    Editor *e = currentEditor();
    if (e) e->toggleFoldAtCursor();
}

void MainWindow::foldCurrentLevel()
{
    Editor *e = currentEditor();
    if (e) e->foldAt(e->textCursor().blockNumber());
}

void MainWindow::unfoldCurrentLevel()
{
    Editor *e = currentEditor();
    if (e) e->unfoldAt(e->textCursor().blockNumber());
}

// Document/Tab operations
void MainWindow::closeOtherTabs()
{
    TabWidget *tw = activeTabWidget();
    int current = tw->currentIndex();

    // Close tabs after current
    for (int i = tw->count() - 1; i > current; --i) {
        closeFile(i);
    }

    // Close tabs before current
    for (int i = current - 1; i >= 0; --i) {
        closeFile(i);
    }
}

void MainWindow::closeTabsToRight()
{
    TabWidget *tw = activeTabWidget();
    int current = tw->currentIndex();

    for (int i = tw->count() - 1; i > current; --i) {
        closeFile(i);
    }
}

void MainWindow::closeTabsToLeft()
{
    TabWidget *tw = activeTabWidget();
    int current = tw->currentIndex();

    // Close tabs at index 0 repeatedly (each close shifts remaining tabs left)
    for (int i = 0; i < current; ++i) {
        closeFile(0);
    }
}

void MainWindow::moveToOtherView()
{
    TabWidget *source = activeTabWidget();
    if (!source || source->count() == 0) return;

    int idx = source->currentIndex();
    Editor *editor = qobject_cast<Editor*>(source->widget(idx));
    if (!editor) return;

    // Ensure split exists
    if (!isSplit()) {
        splitView(Qt::Horizontal);
    }

    TabWidget *target = inactiveTabWidget();
    if (!target) return;

    QString title = source->tabText(idx);
    QIcon icon = source->tabIcon(idx);
    source->removeTab(idx);

    int newIdx = target->addTab(editor, icon, title);
    target->setCurrentIndex(newIdx);
    editor->setFocus();

    checkUnsplitNeeded();
}

void MainWindow::previousTab()
{
    TabWidget *tw = activeTabWidget();
    int current = tw->currentIndex();
    if (current > 0) {
        tw->setCurrentIndex(current - 1);
    } else {
        tw->setCurrentIndex(tw->count() - 1);
    }
}

void MainWindow::nextTab()
{
    TabWidget *tw = activeTabWidget();
    int current = tw->currentIndex();
    if (current < tw->count() - 1) {
        tw->setCurrentIndex(current + 1);
    } else {
        tw->setCurrentIndex(0);
    }
}

// Column/Selection operations
void MainWindow::columnMode()
{
    // TODO: Implement column mode
    statusBar()->showMessage(tr("Column mode not yet implemented"), 3000);
}

void MainWindow::columnEditor()
{
    Editor *editor = currentEditor();
    if (!editor) return;

    ColumnEditor dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;

    if (!editor->isColumnSelectionActive()) {
        QTextCursor cursor = editor->textCursor();
        if (!cursor.hasSelection()) {
            statusBar()->showMessage(
                tr("No column selection active. Use Alt+Click+Drag to select columns."), 3000);
            return;
        }
    }

    if (dialog.mode() == ColumnEditor::TextMode) {
        editor->insertTextAtColumn(dialog.text());
    } else {
        int num = dialog.initialNumber();
        int inc = dialog.increaseBy();
        bool zeros = dialog.leadingZeros();
        editor->insertNumbersAtColumn(num, inc, zeros);
    }
    editor->clearColumnSelection();
}

void MainWindow::beginEndSelect()
{
    // TODO: Implement begin/end select
    statusBar()->showMessage(tr("Begin/End select not yet implemented"), 3000);
}

void MainWindow::launchInTerminal()
{
    Editor *editor = currentEditor();
    if (!editor || !editor->document()) return;
    QString dir = QFileInfo(editor->document()->filePath()).absolutePath();
    if (dir.isEmpty()) dir = QDir::homePath();
    QProcess::startDetached("x-terminal-emulator", QStringList(), dir);
}

void MainWindow::openContainingFolder()
{
    Editor *editor = currentEditor();
    if (!editor || !editor->document()) return;
    QString dir = QFileInfo(editor->document()->filePath()).absolutePath();
    if (!dir.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    }
}

void MainWindow::updateWindowMenu()
{
    m_windowMenu->clear();
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        QString title = m_tabWidget->tabText(i);
        QAction *action = m_windowMenu->addAction(title);
        action->setCheckable(true);
        action->setChecked(i == m_tabWidget->currentIndex());
        connect(action, &QAction::triggered, this, [this, i]() { m_tabWidget->setCurrentIndex(i); });
    }
}

void MainWindow::switchToTab(int index)
{
    if (index >= 0 && index < m_tabWidget->count()) {
        m_tabWidget->setCurrentIndex(index);
    }
}

void MainWindow::toggleAlwaysOnTop()
{
    Qt::WindowFlags flags = windowFlags();
    if (flags & Qt::WindowStaysOnTopHint) {
        setWindowFlags(flags & ~Qt::WindowStaysOnTopHint);
    } else {
        setWindowFlags(flags | Qt::WindowStaysOnTopHint);
    }
    show();
}

void MainWindow::showSummary()
{
    Editor *editor = currentEditor();
    if (!editor) return;
    QString text = editor->rawText();
    int lines = text.count('\n') + 1;
    int words = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).count();
    int chars = text.length();
    QMessageBox::information(this, tr("Summary"),
        tr("Lines: %1\nWords: %2\nCharacters: %3").arg(lines).arg(words).arg(chars));
}

// --- LSP Operations ---

void MainWindow::gotoDefinition()
{
    Editor *e = currentEditor();
    if (e) e->requestGotoDefinition();
}

void MainWindow::findReferences()
{
    Editor *e = currentEditor();
    if (!e || !e->document()) return;

    QString lang = e->document()->language();
    LSPClient *client =
        LSPManager::instance().clientForLanguage(lang);
    if (!client || !client->isInitialized()) return;

    int line = e->currentLine() - 1;
    int col = e->currentColumn() - 1;
    QString uri = QStringLiteral("file://") + e->document()->filePath();
    client->references(uri, line, col);
}

void MainWindow::renameSymbol()
{
    Editor *e = currentEditor();
    if (!e || !e->document()) return;

    QTextCursor cursor = e->textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    QString oldName = cursor.selectedText();

    bool ok = false;
    QString newName = QInputDialog::getText(
        this, tr("Rename Symbol"), tr("New name:"),
        QLineEdit::Normal, oldName, &ok);
    if (!ok || newName.isEmpty() || newName == oldName) return;

    QString lang = e->document()->language();
    LSPClient *client =
        LSPManager::instance().clientForLanguage(lang);
    if (!client || !client->isInitialized()) return;

    int line = e->currentLine() - 1;
    int col = e->currentColumn() - 1;
    QString uri = QStringLiteral("file://") + e->document()->filePath();
    client->rename(uri, line, col, newName);
}

// ============================================================
// LLM Block Evaluation
// ============================================================

void MainWindow::evaluateSelection()
{
    Editor *e = currentEditor();
    if (!e) return;

    QString selectedText = e->selectedText();
    if (selectedText.isEmpty()) {
        statusBar()->showMessage(
            tr("No text selected for evaluation"), 3000);
        return;
    }

    m_evalOriginalText = selectedText;

    if (!m_evalResult) {
        m_evalResult = new EvalResultWidget(e);
        connect(m_evalResult, &EvalResultWidget::accepted,
                this, &MainWindow::onEvalAccepted);
        connect(m_evalResult, &EvalResultWidget::rejected,
                this, &MainWindow::onEvalRejected);
    } else {
        m_evalResult->setParent(e);
    }

    QRect selRect = e->cursorRect();
    m_evalResult->move(selRect.left(), selRect.bottom() + 10);
    m_evalResult->resize(
        e->width() - selRect.left() - 20, 250);
    m_evalResult->showLoading();

    EvalEndpoint endpoint = LLMEvaluator::loadEndpoint();
    m_evaluator->evaluate(selectedText, endpoint);
}

void MainWindow::onEvalResult(const QString &result)
{
    if (m_evalResult) {
        m_evalResult->showResult(m_evalOriginalText, result);
    }
}

void MainWindow::onEvalAccepted(const QString &result)
{
    Editor *e = currentEditor();
    if (e) {
        QTextCursor cursor = e->textCursor();
        cursor.beginEditBlock();
        cursor.insertText(result);
        cursor.endEditBlock();
    }
    if (m_evalResult) m_evalResult->hide();
}

void MainWindow::onEvalRejected()
{
    if (m_evalResult) m_evalResult->hide();
    m_evaluator->cancel();
}

void MainWindow::configureEndpoint()
{
    EndpointConfigDialog dialog(this);
    dialog.setEndpoint(LLMEvaluator::loadEndpoint());
    if (dialog.exec() == QDialog::Accepted) {
        LLMEvaluator::saveEndpoint(dialog.endpoint());
        statusBar()->showMessage(
            tr("Endpoint configuration saved"), 3000);
    }
}

// --- Islamic Knowledge Integration ---

void MainWindow::validateHadith()
{
    Editor *e = currentEditor();
    if (!e) return;

    // Get selected text or current line
    QString text = e->selectedText();
    if (text.isEmpty()) {
        QTextCursor cursor = e->textCursor();
        cursor.select(QTextCursor::BlockUnderCursor);
        text = cursor.selectedText();
    }

    if (text.isEmpty()) return;

    if (!m_islamicBridge) {
        m_islamicBridge = new IslamicBridge(this);
        connect(m_islamicBridge, &IslamicBridge::hadithValidated,
                this, &MainWindow::onHadithValidated);
    }

    statusBar()->showMessage(tr("Validating hadith..."), 0);
    m_islamicBridge->validateHadith(text);
}

void MainWindow::onHadithValidated(const HadithValidation &result)
{
    if (!result.error.isEmpty()) {
        statusBar()->showMessage(
            tr("Hadith validation error: %1")
                .arg(result.error), 5000);
        return;
    }

    QString message;
    if (result.isValid) {
        message = tr("VALID — %1 | Source: %2 | Grading: %3 by %4")
            .arg(result.hadithText.left(50),
                 result.source,
                 result.grading,
                 result.gradingAuthority);
    } else {
        message = tr("NOT VERIFIED — Hadith text could not be "
                      "validated against known collections");
    }

    // Show as tooltip at cursor position
    Editor *e = currentEditor();
    if (e) {
        QToolTip::showText(
            e->mapToGlobal(e->cursorRect().bottomLeft()),
            message, e, QRect(), 10000);
    }
    statusBar()->showMessage(message, 10000);
}

void MainWindow::loadSchemaForDocument()
{
    Editor *e = currentEditor();
    if (!e || !e->document()) return;

    QString filePath = e->document()->filePath();
    if (filePath.isEmpty()) return;

    QFileInfo fi(filePath);
    QString dirName = fi.absoluteDir().dirName();
    QString parentDir =
        QFileInfo(fi.absoluteDir().absolutePath()).dir().dirName();

    // Detect yameen-treatise pipeline files
    bool isYameen =
        filePath.endsWith(".treatise") ||
        filePath.endsWith(".hadith") ||
        dirName == "manuscripts" ||
        parentDir == "yameen-publications" ||
        QFile::exists(
            fi.absoluteDir().filePath("manuscript.yaml")) ||
        QFile::exists(
            fi.absoluteDir().filePath("treatise.yaml"));

    if (isYameen) {
        if (!m_schemaValidator) {
            m_schemaValidator = new SchemaValidator(this);
        }
        m_schemaValidator->loadSchemaFromJson(
            SchemaValidator::hadithSchema());
        validateCurrentDocument();

        if (!m_islamicBridge) {
            m_islamicBridge = new IslamicBridge(this);
            connect(m_islamicBridge,
                    &IslamicBridge::hadithValidated,
                    this, &MainWindow::onHadithValidated);
            m_islamicBridge->testConnection();
        }

        statusBar()->showMessage(
            tr("Yameen Publications pipeline detected"
               " — Islamic schema active"), 5000);
    }
}

void MainWindow::validateCurrentDocument()
{
    if (!m_schemaValidator || !m_schemaValidator->hasSchema())
        return;

    Editor *e = currentEditor();
    if (!e) return;

    QString text = e->rawText();
    QVector<SchemaViolation> violations = m_schemaValidator->validate(text);

    // Update status bar with validation results
    if (violations.isEmpty()) {
        statusBar()->showMessage(tr("Schema: All clear"), 5000);
    } else {
        int errors = 0, warnings = 0;
        for (const auto &v : violations) {
            if (v.severity == SchemaViolation::Error) errors++;
            else if (v.severity == SchemaViolation::Warning) warnings++;
        }
        statusBar()->showMessage(
            QString("Schema: %1 errors, %2 warnings").arg(errors).arg(warnings), 10000);
    }
}

// ============================================================
// Git operations
// ============================================================

void MainWindow::gitCommit()
{
    // Determine repo path from current file or working directory
    QString repoPath;
    Editor *editor = currentEditor();
    if (editor && editor->document() &&
        !editor->document()->filePath().isEmpty()) {
        QFileInfo fi(editor->document()->filePath());
        repoPath = fi.absolutePath();
    } else {
        repoPath = QDir::currentPath();
    }

    // Resolve actual git root
    QProcess proc;
    proc.setWorkingDirectory(repoPath);
    proc.start("git", {"rev-parse", "--show-toplevel"});
    proc.waitForFinished(3000);
    if (proc.exitCode() != 0) {
        QMessageBox::warning(this, tr("Git"),
                             tr("Not inside a Git repository."));
        return;
    }
    repoPath = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();

    auto *dialog = new GitCommitDialog(repoPath, this);
    connect(dialog, &GitCommitDialog::commitCreated,
            this, [this](const QString &hash, const QString &message) {
        statusBar()->showMessage(
            tr("Committed %1: %2").arg(hash, message), 5000);
        updateGitBranch();
    });
    dialog->exec();
    dialog->deleteLater();
}

void MainWindow::gitBranchSwitch()
{
    // Determine repo path
    QString repoPath;
    Editor *editor = currentEditor();
    if (editor && editor->document() &&
        !editor->document()->filePath().isEmpty()) {
        QFileInfo fi(editor->document()->filePath());
        repoPath = fi.absolutePath();
    } else {
        repoPath = QDir::currentPath();
    }

    QProcess proc;
    proc.setWorkingDirectory(repoPath);
    proc.start("git", {"rev-parse", "--show-toplevel"});
    proc.waitForFinished(3000);
    if (proc.exitCode() != 0) {
        QMessageBox::warning(this, tr("Git"),
                             tr("Not inside a Git repository."));
        return;
    }
    repoPath = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();

    // Get branch list
    proc.setWorkingDirectory(repoPath);
    proc.start("git", {"branch", "--list", "--format=%(refname:short)"});
    proc.waitForFinished(5000);
    QStringList branches = QString::fromUtf8(proc.readAllStandardOutput())
                               .trimmed().split('\n', Qt::SkipEmptyParts);

    if (branches.isEmpty()) {
        QMessageBox::information(this, tr("Git"),
                                 tr("No branches found."));
        return;
    }

    // Get current branch for pre-selection
    proc.start("git", {"rev-parse", "--abbrev-ref", "HEAD"});
    proc.waitForFinished(3000);
    QString currentBranch =
        QString::fromUtf8(proc.readAllStandardOutput()).trimmed();

    bool ok;
    QString selected = QInputDialog::getItem(
        this, tr("Switch Branch"), tr("Select branch:"),
        branches, branches.indexOf(currentBranch), false, &ok);

    if (!ok || selected.isEmpty() || selected == currentBranch)
        return;

    proc.start("git", {"checkout", selected});
    proc.waitForFinished(10000);

    if (proc.exitCode() != 0) {
        QString err = QString::fromUtf8(proc.readAllStandardError());
        QMessageBox::warning(this, tr("Git"),
                             tr("Failed to switch branch:\n%1").arg(err));
    } else {
        statusBar()->showMessage(
            tr("Switched to branch: %1").arg(selected), 5000);
        updateGitBranch();
    }
}

void MainWindow::updateGitBranch()
{
    if (!m_gitBranchLabel) return;

    // Determine repo path
    QString repoPath;
    Editor *editor = currentEditor();
    if (editor && editor->document() &&
        !editor->document()->filePath().isEmpty()) {
        QFileInfo fi(editor->document()->filePath());
        repoPath = fi.absolutePath();
    } else {
        repoPath = QDir::currentPath();
    }

    QProcess proc;
    proc.setWorkingDirectory(repoPath);
    proc.start("git", {"rev-parse", "--abbrev-ref", "HEAD"});
    proc.waitForFinished(3000);

    if (proc.exitCode() == 0) {
        QString branch =
            QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        m_gitBranchLabel->setText(branch);
        m_gitBranchLabel->setToolTip(tr("Git branch: %1").arg(branch));
        m_gitBranchLabel->show();
    } else {
        m_gitBranchLabel->hide();
    }
}

// ── Remote file editing ─────────────────────────────────────────────

void MainWindow::openRemoteFile()
{
    auto *dlg = new RemoteConnectionDialog(this);

    connect(dlg, &RemoteConnectionDialog::fileSelected,
            this, &MainWindow::onRemoteFileSelected);

    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
}

void MainWindow::onRemoteFileSelected(
    const RemoteConnection::ConnectionInfo &info,
    const QString &remotePath)
{
    // Build a temp local path for the downloaded file
    QString tempDir = QStandardPaths::writableLocation(
        QStandardPaths::TempLocation);
    QString fileName = QFileInfo(remotePath).fileName();
    QString localPath = tempDir + "/prime-edit-remote-" + fileName;

    // Avoid collisions with already-open remote files
    int suffix = 1;
    while (m_remoteFiles.contains(localPath)) {
        localPath = tempDir + "/prime-edit-remote-"
                    + QString::number(suffix++) + "-" + fileName;
    }

    // Track this as a remote file before download completes
    RemoteFileEntry entry;
    entry.connInfo   = info;
    entry.remotePath = remotePath;
    m_remoteFiles[localPath] = entry;

    // Download via scp then open
    auto *conn = new RemoteConnection(this);
    conn->setConnectionInfo(info);

    connect(conn, &RemoteConnection::fileDownloaded,
            this, [this, conn](const QString &local, const QString &) {
        openFile(local);
        if (m_notificationBar) {
            m_notificationBar->showMessage(
                tr("Opened remote file (auto-uploads on save)"));
        }
        conn->deleteLater();
    });

    connect(conn, &RemoteConnection::connectionError,
            this, [this, conn, localPath](const QString &err) {
        m_remoteFiles.remove(localPath);
        QMessageBox::warning(this, tr("Remote Error"), err);
        conn->deleteLater();
    });

    conn->downloadFile(remotePath, localPath);
}

void MainWindow::uploadRemoteFile(const QString &localPath)
{
    if (!m_remoteFiles.contains(localPath)) return;

    const RemoteFileEntry &entry = m_remoteFiles[localPath];

    auto *conn = new RemoteConnection(this);
    conn->setConnectionInfo(entry.connInfo);

    connect(conn, &RemoteConnection::fileUploaded,
            this, [this, conn](const QString &remote) {
        if (m_notificationBar) {
            m_notificationBar->showMessage(
                tr("Uploaded to %1").arg(remote));
        }
        conn->deleteLater();
    });

    connect(conn, &RemoteConnection::connectionError,
            this, [this, conn](const QString &err) {
        if (m_notificationBar) {
            m_notificationBar->showMessage(
                tr("Upload failed: %1").arg(err));
        }
        conn->deleteLater();
    });

    conn->uploadFile(localPath, entry.remotePath);
}

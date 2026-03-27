// Extended MainWindow implementations for Notepad++ style functionality
// This file contains stub implementations for all the toolbar functions

#include "mainwindow.h"
#include "editor.h"
#include "tabwidget.h"
#include "statusbar.h"
#include "evalresultwidget.h"
#include "endpointconfigdialog.h"
#include "core/document.h"
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
#include <QInputDialog>
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

    QString text = editor->toPlainText();
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

    QString text = editor->toPlainText();
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

    QString text = editor->toPlainText();
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

    QString text = editor->toPlainText();
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

    QString text = editor->toPlainText();
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

    QString text = editor->toPlainText();
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

    QString text = editor->toPlainText();

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

    QString text = editor->toPlainText();
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

    QString text = editor->toPlainText();
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

    QString text = editor->toPlainText();
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

// Encoding operations
void MainWindow::convertToANSI()
{
    Editor *e = currentEditor();
    if (!e || !e->document()) return;
    e->document()->setEncoding("ISO-8859-1");
    m_statusBar->updateFromEditor(e);
    statusBar()->showMessage(tr("Encoding changed to ANSI (ISO-8859-1)"), 3000);
}

void MainWindow::convertToUTF8()
{
    Editor *e = currentEditor();
    if (!e || !e->document()) return;
    e->document()->setEncoding("UTF-8");
    m_statusBar->updateFromEditor(e);
    statusBar()->showMessage(tr("Encoding changed to UTF-8"), 3000);
}

void MainWindow::convertToUTF8BOM()
{
    Editor *e = currentEditor();
    if (!e || !e->document()) return;
    e->document()->setEncoding("UTF-8-BOM");
    m_statusBar->updateFromEditor(e);
    statusBar()->showMessage(tr("Encoding changed to UTF-8-BOM"), 3000);
}

void MainWindow::convertToUCS2BE()
{
    Editor *e = currentEditor();
    if (!e || !e->document()) return;
    e->document()->setEncoding("UTF-16BE");
    m_statusBar->updateFromEditor(e);
    statusBar()->showMessage(tr("Encoding changed to UCS-2 Big Endian"), 3000);
}

void MainWindow::convertToUCS2LE()
{
    Editor *e = currentEditor();
    if (!e || !e->document()) return;
    e->document()->setEncoding("UTF-16LE");
    m_statusBar->updateFromEditor(e);
    statusBar()->showMessage(tr("Encoding changed to UCS-2 Little Endian"), 3000);
}

// Line ending operations
void MainWindow::convertToWindows()
{
    Editor *e = currentEditor();
    if (!e || !e->document()) return;
    e->document()->setLineEnding(Document::Windows);
    m_statusBar->updateFromEditor(e);
    statusBar()->showMessage(tr("Line ending changed to Windows (CRLF)"), 3000);
}

void MainWindow::convertToUnix()
{
    Editor *e = currentEditor();
    if (!e || !e->document()) return;
    e->document()->setLineEnding(Document::Unix);
    m_statusBar->updateFromEditor(e);
    statusBar()->showMessage(tr("Line ending changed to Unix (LF)"), 3000);
}

void MainWindow::convertToMac()
{
    Editor *e = currentEditor();
    if (!e || !e->document()) return;
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
    int current = m_tabWidget->currentIndex();

    // Close tabs after current
    for (int i = m_tabWidget->count() - 1; i > current; --i) {
        closeFile(i);
    }

    // Close tabs before current
    for (int i = current - 1; i >= 0; --i) {
        closeFile(i);
    }
}

void MainWindow::closeTabsToRight()
{
    int current = m_tabWidget->currentIndex();

    for (int i = m_tabWidget->count() - 1; i > current; --i) {
        closeFile(i);
    }
}

void MainWindow::closeTabsToLeft()
{
    int current = m_tabWidget->currentIndex();

    for (int i = current - 1; i >= 0; --i) {
        closeFile(i);
    }
}

void MainWindow::moveToOtherView()
{
    // TODO: Implement move to other view (split view)
    statusBar()->showMessage(tr("Move to other view not yet implemented"), 3000);
}

void MainWindow::cloneToOtherView()
{
    // TODO: Implement clone to other view (split view)
    statusBar()->showMessage(tr("Clone to other view not yet implemented"), 3000);
}

void MainWindow::previousTab()
{
    int current = m_tabWidget->currentIndex();
    if (current > 0) {
        m_tabWidget->setCurrentIndex(current - 1);
    } else {
        m_tabWidget->setCurrentIndex(m_tabWidget->count() - 1);
    }
}

void MainWindow::nextTab()
{
    int current = m_tabWidget->currentIndex();
    if (current < m_tabWidget->count() - 1) {
        m_tabWidget->setCurrentIndex(current + 1);
    } else {
        m_tabWidget->setCurrentIndex(0);
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
    if (dialog.exec() == QDialog::Accepted) {
        // Apply column editor result
        if (dialog.mode() == ColumnEditor::TextMode) {
            editor->insertTextAtColumn(dialog.text());
        }
        // Number mode would need column selection active
    }
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
    QString text = editor->toPlainText();
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

    QString text = e->toPlainText();
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

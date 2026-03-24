#include "editor.h"
#include "linenumberarea.h"
#include "core/document.h"
#include "core/macrorecorder.h"
#include "syntax/highlighter.h"
#include "utils/settings.h"

#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>

Editor::Editor(Document *document, QWidget *parent)
    : QPlainTextEdit(parent)
    , m_document(document)
{
    setupEditor();
    applySettings();

    // Create line number area
    m_lineNumberArea = new LineNumberArea(this);

    // Create syntax highlighter (using QPlainTextEdit's document)
    m_highlighter = new SyntaxHighlighter(QPlainTextEdit::document());
    if (!m_document->language().isEmpty()) {
        m_highlighter->setLanguage(m_document->language());
    }

    // Connect signals (but NOT textChanged yet - we'll connect that after loading content)
    connect(this, &QPlainTextEdit::blockCountChanged, this, &Editor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &Editor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &Editor::highlightCurrentLine);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &Editor::cursorPositionChanged);

    connect(m_document, &Document::languageChanged, this, [this](const QString &lang) {
        m_highlighter->setLanguage(lang);
    });

    // Connect settings
    connect(&Settings::instance(), &Settings::fontChanged, this, &Editor::applySettings);
    connect(&Settings::instance(), &Settings::tabSettingsChanged, this, &Editor::applySettings);

    // Initialize
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    // Load content from document BEFORE connecting textChanged signal
    if (!m_document->text().isEmpty()) {
        setPlainText(m_document->text());
    }

    // NOW connect textChanged - after initial content is loaded
    connect(this, &QPlainTextEdit::textChanged, this, &Editor::onTextChanged);
}

Editor::~Editor()
{
}

void Editor::setupEditor()
{
    // Basic setup
    setFrameShape(QFrame::NoFrame);
    setLineWrapMode(Settings::instance().wordWrap() ? WidgetWidth : NoWrap);

    // Set monospace font
    QFont font = Settings::instance().font();
    setFont(font);
    m_baseFontSize = font.pointSize();

    // Tab settings
    m_tabWidth = Settings::instance().tabWidth();
    m_insertSpaces = Settings::instance().insertSpaces();
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * m_tabWidth);
}

void Editor::applySettings()
{
    QFont font = Settings::instance().font();
    font.setPointSize(m_baseFontSize + m_zoomLevel);
    setFont(font);

    m_tabWidth = Settings::instance().tabWidth();
    m_insertSpaces = Settings::instance().insertSpaces();
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * m_tabWidth);

    updateLineNumberAreaWidth(0);
}

int Editor::lineNumberAreaWidth() const
{
    if (!m_lineNumbersVisible) {
        return 0;
    }

    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void Editor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    if (!m_lineNumbersVisible) {
        return;
    }

    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor(45, 45, 45));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor(150, 150, 150));

            // Highlight current line number
            if (blockNumber == textCursor().blockNumber()) {
                painter.setPen(QColor(255, 255, 255));
            }

            painter.drawText(0, top, m_lineNumberArea->width() - 5, fontMetrics().height(),
                            Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void Editor::setLineNumbersVisible(bool visible)
{
    m_lineNumbersVisible = visible;
    updateLineNumberAreaWidth(0);
}

void Editor::setWordWrapEnabled(bool enabled)
{
    setLineWrapMode(enabled ? WidgetWidth : NoWrap);
}

bool Editor::wordWrapEnabled() const
{
    return lineWrapMode() == WidgetWidth;
}

void Editor::setTabWidth(int width)
{
    m_tabWidth = width;
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * m_tabWidth);
}

void Editor::setInsertSpaces(bool insert)
{
    m_insertSpaces = insert;
}

void Editor::zoomIn(int range)
{
    m_zoomLevel += range;
    QFont f = font();
    f.setPointSize(m_baseFontSize + m_zoomLevel);
    setFont(f);
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * m_tabWidth);
    updateLineNumberAreaWidth(0);
}

void Editor::zoomOut(int range)
{
    if (m_baseFontSize + m_zoomLevel - range > 4) {
        m_zoomLevel -= range;
        QFont f = font();
        f.setPointSize(m_baseFontSize + m_zoomLevel);
        setFont(f);
        setTabStopDistance(fontMetrics().horizontalAdvance(' ') * m_tabWidth);
        updateLineNumberAreaWidth(0);
    }
}

void Editor::resetZoom()
{
    m_zoomLevel = 0;
    QFont f = font();
    f.setPointSize(m_baseFontSize);
    setFont(f);
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * m_tabWidth);
    updateLineNumberAreaWidth(0);
}

void Editor::goToLine(int line)
{
    QTextBlock block = QPlainTextEdit::document()->findBlockByLineNumber(line - 1);
    if (block.isValid()) {
        QTextCursor cursor(block);
        setTextCursor(cursor);
        centerCursor();
    }
}

int Editor::currentLine() const
{
    return textCursor().blockNumber() + 1;
}

int Editor::currentColumn() const
{
    return textCursor().columnNumber() + 1;
}

bool Editor::findNext(const QString &text, QTextDocument::FindFlags flags)
{
    if (text.isEmpty()) {
        return false;
    }

    QTextCursor cursor = QPlainTextEdit::document()->find(text, textCursor(), flags);
    if (cursor.isNull()) {
        // Wrap around
        cursor = QPlainTextEdit::document()->find(text, 0, flags);
    }

    if (!cursor.isNull()) {
        setTextCursor(cursor);
        return true;
    }
    return false;
}

bool Editor::findPrevious(const QString &text, QTextDocument::FindFlags flags)
{
    return findNext(text, flags | QTextDocument::FindBackward);
}

int Editor::replaceAll(const QString &findText, const QString &replaceText, QTextDocument::FindFlags flags)
{
    if (findText.isEmpty()) {
        return 0;
    }

    int count = 0;
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    cursor.movePosition(QTextCursor::Start);
    setTextCursor(cursor);

    while (findNext(findText, flags)) {
        textCursor().insertText(replaceText);
        ++count;
    }

    cursor.endEditBlock();
    return count;
}

QString Editor::selectedText() const
{
    return textCursor().selectedText();
}

void Editor::replaceSelection(const QString &text)
{
    textCursor().insertText(text);
}

void Editor::setLanguage(const QString &language)
{
    m_highlighter->setLanguage(language);
    m_document->setLanguage(language);
}

QString Editor::language() const
{
    return m_document->language();
}

void Editor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void Editor::keyPressEvent(QKeyEvent *event)
{
    // Record macro event
    MacroRecorder::instance().recordKeyEvent(event);

    // Handle tab key
    if (event->key() == Qt::Key_Tab) {
        if (m_insertSpaces) {
            QTextCursor cursor = textCursor();
            int column = cursor.columnNumber();
            int spacesToInsert = m_tabWidth - (column % m_tabWidth);
            cursor.insertText(QString(spacesToInsert, ' '));
            return;
        }
    }

    // Handle backtab (shift+tab)
    if (event->key() == Qt::Key_Backtab) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::StartOfBlock);
        QString blockText = cursor.block().text();

        // Count leading whitespace to remove
        int spacesToRemove = 0;
        for (int i = 0; i < qMin(m_tabWidth, static_cast<int>(blockText.length())); ++i) {
            if (blockText[i] == ' ') {
                ++spacesToRemove;
            } else if (blockText[i] == '\t') {
                spacesToRemove = i + 1;
                break;
            } else {
                break;
            }
        }

        if (spacesToRemove > 0) {
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, spacesToRemove);
            cursor.removeSelectedText();
        }
        return;
    }

    // Handle enter key for auto-indent
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QTextCursor cursor = textCursor();
        QString blockText = cursor.block().text();

        // Get leading whitespace
        QString indent;
        for (const QChar &ch : blockText) {
            if (ch == ' ' || ch == '\t') {
                indent += ch;
            } else {
                break;
            }
        }

        cursor.insertText("\n" + indent);
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
}

void Editor::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        const int delta = event->angleDelta().y();
        if (delta > 0) {
            zoomIn();
        } else if (delta < 0) {
            zoomOut();
        }
        return;
    }
    QPlainTextEdit::wheelEvent(event);
}

void Editor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void Editor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy) {
        m_lineNumberArea->scroll(0, dy);
    } else {
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void Editor::highlightCurrentLine()
{
    // Prevent recursive calls
    static bool highlighting = false;
    if (highlighting) {
        return;
    }
    highlighting = true;

    if (!Settings::instance().highlightCurrentLine()) {
        setExtraSelections({});
        highlighting = false;
        return;
    }

    QList<QTextEdit::ExtraSelection> extraSelections;

    QTextEdit::ExtraSelection selection;
    QColor lineColor = QColor(230, 230, 230);  // Light gray for current line

    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    extraSelections.append(selection);

    setExtraSelections(extraSelections);
    highlighting = false;
}

void Editor::onTextChanged()
{
    // Prevent recursive calls
    static bool inTextChanged = false;
    if (inTextChanged) {
        return;
    }
    inTextChanged = true;

    // Mark document as modified
    if (!m_syncing) {
        m_document->setModified(true);
    }

    inTextChanged = false;
}

void Editor::syncToDocument()
{
    // Sync editor content to document (called before save)
    m_syncing = true;
    m_document->content()->setText(toPlainText());
    m_syncing = false;
}

void Editor::syncFromDocument()
{
    // Sync document content to editor (called after load)
    m_syncing = true;
    setPlainText(m_document->text());
    m_syncing = false;
}

QString Editor::indentString() const
{
    if (m_insertSpaces) {
        return QString(m_tabWidth, ' ');
    }
    return "\t";
}

// Line operations

void Editor::duplicateLine()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    // Select entire line(s)
    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();

    cursor.setPosition(start);
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

    QString text = cursor.selectedText();
    // Replace paragraph separators with newlines
    text.replace(QChar::ParagraphSeparator, '\n');

    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.insertText("\n" + text);

    cursor.endEditBlock();
}

void Editor::deleteLine()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);

    if (cursor.atEnd() && !cursor.hasSelection()) {
        // Last line - delete to start of block
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    }

    cursor.removeSelectedText();
    cursor.endEditBlock();
    setTextCursor(cursor);
}

void Editor::moveLineUp()
{
    QTextCursor cursor = textCursor();
    if (cursor.blockNumber() == 0) {
        return; // Already at first line
    }

    cursor.beginEditBlock();

    // Get current line
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    QString currentLine = cursor.selectedText();

    // Delete current line and newline before it
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();

    // Move to previous line and insert
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.insertText(currentLine + "\n");
    cursor.movePosition(QTextCursor::PreviousBlock);

    cursor.endEditBlock();
    setTextCursor(cursor);
}

void Editor::moveLineDown()
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::EndOfBlock);
    if (cursor.atEnd()) {
        return; // Already at last line
    }

    cursor.beginEditBlock();

    // Get current line
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    QString currentLine = cursor.selectedText();

    // Delete current line and newline after it
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();

    // Move to end of next line and insert
    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.insertText("\n" + currentLine);

    cursor.endEditBlock();
    setTextCursor(cursor);
}

QString Editor::getCommentString() const
{
    QString lang = m_document->language().toLower();

    if (lang == "python" || lang == "py" || lang == "bash" || lang == "sh" ||
        lang == "shell" || lang == "yaml" || lang == "yml" || lang == "ruby") {
        return "# ";
    } else if (lang == "c++" || lang == "cpp" || lang == "c" || lang == "java" ||
               lang == "javascript" || lang == "js" || lang == "typescript" || lang == "ts" ||
               lang == "rust" || lang == "go" || lang == "css" || lang == "scss") {
        return "// ";
    } else if (lang == "html" || lang == "xml") {
        return "<!-- ";  // Note: HTML comments need closing tag
    } else if (lang == "sql") {
        return "-- ";
    } else if (lang == "lua") {
        return "-- ";
    }
    return "// ";  // Default
}

void Editor::toggleComment()
{
    QString commentStr = getCommentString();
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();

    cursor.setPosition(start);
    int startBlock = cursor.blockNumber();
    cursor.setPosition(end);
    int endBlock = cursor.blockNumber();

    // Check if all lines are commented
    bool allCommented = true;
    cursor.setPosition(start);
    for (int i = startBlock; i <= endBlock; ++i) {
        cursor.movePosition(QTextCursor::StartOfBlock);
        QString lineText = cursor.block().text();
        QString trimmed = lineText.trimmed();
        if (!trimmed.isEmpty() && !trimmed.startsWith(commentStr.trimmed())) {
            allCommented = false;
            break;
        }
        cursor.movePosition(QTextCursor::NextBlock);
    }

    // Toggle comments
    cursor.setPosition(start);
    for (int i = startBlock; i <= endBlock; ++i) {
        cursor.movePosition(QTextCursor::StartOfBlock);
        QString lineText = cursor.block().text();

        if (allCommented) {
            // Remove comment
            int commentPos = lineText.indexOf(commentStr.trimmed());
            if (commentPos >= 0) {
                cursor.setPosition(cursor.block().position() + commentPos);
                cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, commentStr.trimmed().length());
                if (cursor.selectedText() == commentStr.trimmed()) {
                    // Also remove trailing space if present
                    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                    if (cursor.selectedText().endsWith(' ')) {
                        cursor.removeSelectedText();
                    } else {
                        cursor.setPosition(cursor.selectionStart());
                        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, commentStr.trimmed().length());
                        cursor.removeSelectedText();
                    }
                }
            }
        } else {
            // Add comment at start of line (after leading whitespace)
            int firstNonSpace = 0;
            for (int j = 0; j < lineText.length(); ++j) {
                if (!lineText[j].isSpace()) {
                    firstNonSpace = j;
                    break;
                }
            }
            cursor.setPosition(cursor.block().position() + firstNonSpace);
            cursor.insertText(commentStr);
        }
        cursor.movePosition(QTextCursor::NextBlock);
    }

    cursor.endEditBlock();
}

void Editor::sortLinesAscending()
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        return;
    }

    cursor.beginEditBlock();

    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();

    cursor.setPosition(start);
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

    QString text = cursor.selectedText();
    text.replace(QChar::ParagraphSeparator, '\n');
    QStringList lines = text.split('\n');
    lines.sort(Qt::CaseInsensitive);

    cursor.insertText(lines.join('\n'));
    cursor.endEditBlock();
}

void Editor::sortLinesDescending()
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        return;
    }

    cursor.beginEditBlock();

    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();

    cursor.setPosition(start);
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

    QString text = cursor.selectedText();
    text.replace(QChar::ParagraphSeparator, '\n');
    QStringList lines = text.split('\n');
    lines.sort(Qt::CaseInsensitive);
    std::reverse(lines.begin(), lines.end());

    cursor.insertText(lines.join('\n'));
    cursor.endEditBlock();
}

void Editor::removeDuplicateLines()
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        return;
    }

    cursor.beginEditBlock();

    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();

    cursor.setPosition(start);
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

    QString text = cursor.selectedText();
    text.replace(QChar::ParagraphSeparator, '\n');
    QStringList lines = text.split('\n');

    QStringList uniqueLines;
    QSet<QString> seen;
    for (const QString &line : lines) {
        if (!seen.contains(line)) {
            seen.insert(line);
            uniqueLines.append(line);
        }
    }

    cursor.insertText(uniqueLines.join('\n'));
    cursor.endEditBlock();
}

void Editor::trimTrailingWhitespace()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    cursor.movePosition(QTextCursor::Start);
    while (!cursor.atEnd()) {
        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        QString lineText = cursor.selectedText();

        int trimmedLength = lineText.trimmed().isEmpty() ? 0 : lineText.length();
        while (trimmedLength > 0 && lineText[trimmedLength - 1].isSpace()) {
            --trimmedLength;
        }

        if (trimmedLength < lineText.length()) {
            cursor.movePosition(QTextCursor::StartOfBlock);
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, trimmedLength);
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
        }

        cursor.movePosition(QTextCursor::NextBlock);
    }

    cursor.endEditBlock();
}

void Editor::joinLines()
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        // Join current line with next
        cursor.movePosition(QTextCursor::EndOfBlock);
        if (!cursor.atEnd()) {
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            cursor.insertText(" ");
        }
        return;
    }

    cursor.beginEditBlock();

    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();

    cursor.setPosition(start);
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

    QString text = cursor.selectedText();
    text.replace(QChar::ParagraphSeparator, ' ');
    text = text.simplified();

    cursor.insertText(text);
    cursor.endEditBlock();
}

void Editor::toUpperCase()
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        return;
    }

    QString text = cursor.selectedText();
    cursor.insertText(text.toUpper());
}

void Editor::toLowerCase()
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        return;
    }

    QString text = cursor.selectedText();
    cursor.insertText(text.toLower());
}

void Editor::toTitleCase()
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        return;
    }

    QString text = cursor.selectedText();
    bool capitalizeNext = true;
    for (int i = 0; i < text.length(); ++i) {
        if (text[i].isLetter()) {
            if (capitalizeNext) {
                text[i] = text[i].toUpper();
                capitalizeNext = false;
            } else {
                text[i] = text[i].toLower();
            }
        } else if (text[i].isSpace() || text[i] == '-' || text[i] == '_') {
            capitalizeNext = true;
        }
    }
    cursor.insertText(text);
}

// Code folding

void Editor::setFoldingEnabled(bool enabled)
{
    m_foldingEnabled = enabled;
    // Note: Full folding implementation requires custom gutter widget
    // This is a simplified version
}

void Editor::foldAll()
{
    // Simplified folding - would need proper implementation with fold regions
    QTextBlock block = QPlainTextEdit::document()->begin();
    while (block.isValid()) {
        block.setVisible(true);
        block = block.next();
    }
}

void Editor::unfoldAll()
{
    QTextBlock block = QPlainTextEdit::document()->begin();
    while (block.isValid()) {
        block.setVisible(true);
        block = block.next();
    }
    viewport()->update();
}

void Editor::toggleFoldAtCursor()
{
    // Simplified - would need proper fold region detection
    QTextCursor cursor = textCursor();
    QTextBlock block = cursor.block();
    block.setVisible(!block.isVisible());
    viewport()->update();
}

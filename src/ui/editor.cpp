#include "editor.h"
#include "linenumberarea.h"
#include "theme.h"
#include "completionpopup.h"
#include "core/document.h"
#include "core/largefile.h"
#include "core/macrorecorder.h"
#include "core/lspmanager.h"
#include "core/lspclient.h"
#include "syntax/highlighter.h"
#include "utils/settings.h"

#include <QPainter>
#include <QTextBlock>
#include <QTextLayout>
#include <QScrollBar>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QRegularExpression>
#include <QToolTip>
#include <QSet>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <algorithm>

Editor::Editor(Document *document, QWidget *parent)
    : QPlainTextEdit(parent)
    , m_document(document)
{
    setupEditor();
    applySettings();

    // Create line number area
    m_lineNumberArea = new LineNumberArea(this);

    // Git gutter — debounced update on text changes
    m_gitGutter = new GitGutter(this);
    m_gitGutterTimer = new QTimer(this);
    m_gitGutterTimer->setSingleShot(true);
    m_gitGutterTimer->setInterval(1000);
    connect(m_gitGutterTimer, &QTimer::timeout,
            this, &Editor::updateGitGutter);
    connect(m_gitGutter, &GitGutter::gutterUpdated,
            m_lineNumberArea, QOverload<>::of(&QWidget::update));
    connect(this, &QPlainTextEdit::textChanged, this, [this]() {
        m_gitGutterTimer->start();
    });

    // Create syntax highlighter — but DON'T set language yet for large files
    // (will be set after content is loaded to avoid highlighting empty document
    // then re-highlighting after setPlainText)
    m_highlighter = new SyntaxHighlighter(QPlainTextEdit::document());

    // Connect signals (but NOT textChanged yet - we'll connect that after loading content)
    connect(this, &QPlainTextEdit::blockCountChanged, this, &Editor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &Editor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &Editor::highlightCurrentLine);

    connect(m_document, &Document::languageChanged, this, [this](const QString &lang) {
        m_highlighter->setLanguage(lang);
    });

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
        m_highlighter->setupFormats();
        m_highlighter->rehighlight();
    });

    // Connect settings
    connect(&Settings::instance(), &Settings::fontChanged, this, &Editor::applySettings);
    connect(&Settings::instance(), &Settings::tabSettingsChanged, this, &Editor::applySettings);

    // Configure file mode BEFORE loading content
    if (m_document->fileMode() == Document::LargeFile) {
        // CRITICAL: set word wrap BEFORE setPlainText — without wrap,
        // QPlainTextEdit tries to lay out the full width of every line.
        // A 93-million-character single line causes an infinite hang.
        // With wrap ON, it only lays out the visible wrapped portion.
        setLineWrapMode(QPlainTextEdit::WidgetWidth);
        setWordWrapMode(QTextOption::WrapAnywhere);
        QPlainTextEdit::document()->setUndoRedoEnabled(false);
        m_foldMarginVisible = false;
        m_bookmarkMarginVisible = false;
    } else if (m_document->fileMode() == Document::MediumFile) {
        QPlainTextEdit::document()->setUndoRedoEnabled(true);
    }

    // Initialize
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    // Load content
    m_syncing = true;
    if (m_document->fileMode() == Document::LargeFile) {
        // Large file: load first 2MB only. QTextDocument cannot handle
        // 93MB in any form — full load, chunked load, WrapAnywhere all
        // cause 2.5GB RSS and crash. 2MB preview is the only stable approach.
        setUpdatesEnabled(false);
        QFile file(m_document->filePath());
        if (file.open(QIODevice::ReadOnly)) {
            constexpr qint64 PREVIEW_SIZE = 2 * 1024 * 1024;
            QByteArray raw = file.read(PREVIEW_SIZE);
            bool truncated = !file.atEnd();
            qint64 totalSize = file.size();
            file.close();

            QString text = QString::fromUtf8(raw);
            raw.clear();

            // For minified files: break long lines so QTextDocument doesn't choke
            if (m_document->isMinified()) {
                constexpr int LINE_LEN = 1000;
                QString broken;
                broken.reserve(text.length() + text.length() / LINE_LEN);
                for (qsizetype i = 0; i < text.length(); i += LINE_LEN) {
                    if (i > 0) broken.append('\n');
                    broken.append(text.mid(i, LINE_LEN));
                }
                text = broken;
            }

            if (truncated) {
                text.append(QString("\n\n[ Showing first %1 of %2 — file is read-only ]")
                    .arg(QLocale().formattedDataSize(PREVIEW_SIZE))
                    .arg(QLocale().formattedDataSize(totalSize)));
            }

            setPlainText(text);
        }
        setUpdatesEnabled(true);
        QPlainTextEdit::document()->setModified(false);
        m_document->setModified(false);
    } else if (!m_document->text().isEmpty()) {
        setPlainText(m_document->text());
    }
    m_syncing = false;

    // Set language — skip syntax highlighting if file is over 5MB
    // (regex on thousands of 2000+ char lines causes noticeable lag)
    if (!m_document->language().isEmpty()) {
        qint64 size = QFileInfo(m_document->filePath()).size();
        if (size < 5 * 1024 * 1024 && m_document->fileMode() == Document::SmallFile) {
            m_highlighter->setLanguage(m_document->language());
        }
    }

    // Large file: set up dynamic viewport loading via scrollbar
    if (m_document->isLargeFile()) {
        connect(verticalScrollBar(), &QScrollBar::valueChanged,
                this, &Editor::loadViewportContent);

        // Set scrollbar range based on estimated line count
        // The actual count will be updated when the background index completes
        LargeFileReader *reader = m_document->largeFileReader();
        if (reader) {
            int lineCount = static_cast<int>(qMin(reader->lineCount(), qint64(INT_MAX)));
            verticalScrollBar()->setRange(0, qMax(1, lineCount - 1));

            // When the background index finishes, update the scrollbar range
            connect(reader, &LargeFileReader::indexBuildComplete, this, [this]() {
                LargeFileReader *r = m_document->largeFileReader();
                if (r) {
                    int lc = static_cast<int>(qMin(r->lineCount(), qint64(INT_MAX)));
                    verticalScrollBar()->setRange(0, qMax(1, lc - 1));
                }
            });
        }
    }

    // Modification detection: track whether the document has been edited by the user.
    // Use QTextDocument::modificationChanged which Qt manages correctly —
    // it only fires for real content changes, not formatting.
    QPlainTextEdit::document()->setModified(false);
    connect(QPlainTextEdit::document(), &QTextDocument::modificationChanged,
            this, [this](bool changed) {
        if (!m_syncing) {
            m_document->setModified(changed);
        }
    });

    // Clear stale multi-cursors on undo/redo and invalidate rainbow bracket cache
    connect(QPlainTextEdit::document(), &QTextDocument::contentsChanged,
            this, [this]() {
        m_cachedRainbowFirstBlock = -1;
        if (!m_inMultiCursorEdit && !m_extraCursors.isEmpty()) {
            clearExtraCursors();
        }
    });
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
    font.setStyleHint(QFont::Monospace);
    // Only add Arabic font fallbacks if they won't override the primary monospace font
    // Qt resolves families in order — keep monospace fonts first
    QStringList families;
    families << font.family() << "DejaVu Sans Mono" << "Liberation Mono" << "Courier New";
    // Arabic fallbacks last (for mixed content documents)
    families << "Noto Sans Arabic" << "Amiri";
    font.setFamilies(families);
    setFont(font);
    m_baseFontSize = font.pointSize();

    // Tab settings
    m_tabWidth = Settings::instance().tabWidth();
    m_insertSpaces = Settings::instance().insertSpaces();
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * m_tabWidth);

    // Debounced auto-completion trigger (300ms after last keystroke)
    m_completionTimer = new QTimer(this);
    m_completionTimer->setSingleShot(true);
    m_completionTimer->setInterval(300);
    connect(m_completionTimer, &QTimer::timeout,
            this, &Editor::triggerCompletion);

    // Debounced Arabic RTL direction update
    m_rtlTimer = new QTimer(this);
    m_rtlTimer->setSingleShot(true);
    m_rtlTimer->setInterval(2000);
    connect(m_rtlTimer, &QTimer::timeout,
            this, &Editor::updateTextDirection);
    connect(this, &QPlainTextEdit::textChanged, this, [this]() {
        m_rtlTimer->start();
    });

}

void Editor::applySettings()
{
    QFont font = Settings::instance().font();
    font.setPointSize(m_baseFontSize + m_zoomLevel);
    font.setStyleHint(QFont::Monospace);
    QStringList families;
    families << font.family() << "DejaVu Sans Mono" << "Liberation Mono" << "Courier New";
    families << "Noto Sans Arabic" << "Amiri";
    font.setFamilies(families);
    setFont(font);

    m_tabWidth = Settings::instance().tabWidth();
    m_insertSpaces = Settings::instance().insertSpaces();
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * m_tabWidth);

    updateLineNumberAreaWidth(0);
}

void Editor::updateGitGutter()
{
    if (!m_document || m_document->filePath().isEmpty())
        return;
    m_gitGutter->updateForFile(m_document->filePath(),
                               toPlainText());
}

void Editor::updateTextDirection()
{
    // Skip for medium/large files — scanning every block is too expensive
    if (m_document && m_document->fileMode() != Document::SmallFile) return;

    QTextBlock block = QPlainTextEdit::document()->begin();
    bool foundArabic = false;
    bool wasModified = m_document->isModified();

    // Block signals to prevent setBlockFormat from triggering textChanged → setModified
    bool oldSyncing = m_syncing;
    m_syncing = true;

    while (block.isValid()) {
        QString text = block.text().trimmed();
        if (!text.isEmpty()) {
            bool hasArabic = false;
            for (const QChar &ch : text) {
                ushort code = ch.unicode();
                if (code >= 0x0600 && code <= 0x06FF) {
                    hasArabic = true;
                    foundArabic = true;
                    break;
                }
            }

            QTextBlockFormat fmt = block.blockFormat();
            Qt::LayoutDirection dir =
                hasArabic ? Qt::RightToLeft : Qt::LeftToRight;
            if (fmt.layoutDirection() != dir) {
                QTextCursor cursor(block);
                fmt.setLayoutDirection(dir);
                cursor.setBlockFormat(fmt);
            }
        }
        block = block.next();
    }

    m_syncing = oldSyncing;

    // Restore original modified state — RTL detection should not mark file as modified
    if (!wasModified) {
        m_document->setModified(false);
    }

    m_hasArabicContent = foundArabic;
}

int Editor::lineNumberAreaWidth() const
{
    int width = 0;
    // Git gutter stripe at left edge of bookmark margin
    if (m_gitGutter && m_gitGutter->hasChanges())
        width += GIT_GUTTER_WIDTH;
    if (m_bookmarkMarginVisible) width += BOOKMARK_MARGIN_WIDTH;
    if (m_lineNumbersVisible) {
        int digits = 1;
        int max = qMax(1, blockCount());
        while (max >= 10) { max /= 10; ++digits; }
        width += 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    }
    if (m_foldMarginVisible) width += FOLD_MARGIN_WIDTH;
    return width;
}

int Editor::bookmarkMarginWidth() const
{
    return m_bookmarkMarginVisible ? BOOKMARK_MARGIN_WIDTH : 0;
}

int Editor::foldMarginWidth() const
{
    return m_foldMarginVisible ? FOLD_MARGIN_WIDTH : 0;
}

void Editor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    Theme theme = ThemeManager::instance().currentTheme();
    QPainter painter(m_lineNumberArea);

    // Git gutter occupies the leftmost strip when changes exist
    int gitWidth = (m_gitGutter && m_gitGutter->hasChanges())
                       ? GIT_GUTTER_WIDTH : 0;
    int bmWidth = bookmarkMarginWidth();
    int fmWidth = foldMarginWidth();

    // Calculate line number section width
    int lineNumWidth = 0;
    if (m_lineNumbersVisible) {
        int digits = 1;
        int max = qMax(1, blockCount());
        while (max >= 10) { max /= 10; ++digits; }
        lineNumWidth = 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    }

    // Paint section backgrounds
    if (gitWidth > 0) {
        QRect gitRect(0, event->rect().top(), gitWidth, event->rect().height());
        painter.fillRect(gitRect, theme.lineNumberBackground);
    }
    if (m_bookmarkMarginVisible) {
        QRect bmRect(gitWidth, event->rect().top(), bmWidth, event->rect().height());
        painter.fillRect(bmRect, theme.bookmarkMarginBackground);
    }
    if (m_lineNumbersVisible) {
        QRect lnRect(gitWidth + bmWidth, event->rect().top(), lineNumWidth, event->rect().height());
        painter.fillRect(lnRect, theme.lineNumberBackground);
    }
    if (m_foldMarginVisible) {
        QRect fmRect(gitWidth + bmWidth + lineNumWidth, event->rect().top(), fmWidth, event->rect().height());
        painter.fillRect(fmRect, theme.foldMarginBackground);
    }

    // Paint per-block elements
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            int lineHeight = qRound(blockBoundingRect(block).height());
            int lineNum1Based = blockNumber + 1;

            // Section 0: Git gutter indicators
            if (gitWidth > 0 && m_gitGutter->hasChangeAt(lineNum1Based)) {
                GitLineChange::Type changeType =
                    m_gitGutter->lineStatus(lineNum1Based);
                QColor color;
                switch (changeType) {
                case GitLineChange::Added:
                    color = QColor(0x2E, 0xCC, 0x40); // green
                    break;
                case GitLineChange::Modified:
                    color = QColor(0xFF, 0xDC, 0x00); // yellow
                    break;
                case GitLineChange::Deleted:
                    color = QColor(0xFF, 0x41, 0x36); // red
                    break;
                }
                if (changeType == GitLineChange::Deleted) {
                    painter.setRenderHint(QPainter::Antialiasing, true);
                    painter.setBrush(color);
                    painter.setPen(Qt::NoPen);
                    QPolygon tri;
                    tri << QPoint(0, top)
                        << QPoint(gitWidth, top)
                        << QPoint(gitWidth / 2, top + 4);
                    painter.drawPolygon(tri);
                    painter.setRenderHint(QPainter::Antialiasing, false);
                } else {
                    painter.fillRect(0, top, gitWidth, lineHeight, color);
                }
            }

            // Section 1: Bookmark indicators
            if (m_bookmarkMarginVisible && m_bookmarks.contains(blockNumber + 1)) {
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setBrush(QColor(0x33, 0x99, 0xFF));
                painter.setPen(Qt::NoPen);
                int cx = gitWidth + bmWidth / 2;
                int cy = top + lineHeight / 2;
                painter.drawEllipse(QPoint(cx, cy), 5, 5);
                painter.setRenderHint(QPainter::Antialiasing, false);
            }

            // Section 2: Line numbers
            if (m_lineNumbersVisible) {
                int displayLineNumber = blockNumber + 1;
                if (m_document && m_document->isLargeFile()) {
                    displayLineNumber = static_cast<int>(blockNumber + m_viewportStartLine + 1);
                }
                QString number = QString::number(displayLineNumber);
                painter.setPen(theme.lineNumberForeground);
                if (blockNumber == textCursor().blockNumber()) {
                    painter.setPen(theme.foreground);
                }
                painter.drawText(gitWidth + bmWidth, top, lineNumWidth - 5, fontMetrics().height(),
                                Qt::AlignRight, number);
            }

            // Section 3: Fold margin indicators
            if (m_foldMarginVisible) {
                int foldX = gitWidth + bmWidth + lineNumWidth;
                if (isFoldableLine(blockNumber) ||
                    isFoldedLine(blockNumber)) {
                    int midY = top + (bottom - top) / 2;
                    int boxSize = 9;
                    int boxX = foldX + (FOLD_MARGIN_WIDTH - boxSize) / 2;
                    int boxY = midY - boxSize / 2;

                    painter.setPen(QPen(QColor(128, 128, 128)));
                    painter.setBrush(theme.foldMarginBackground);
                    painter.drawRect(boxX, boxY, boxSize, boxSize);

                    // Horizontal minus sign (always present)
                    painter.drawLine(
                        boxX + 2, midY,
                        boxX + boxSize - 2, midY);

                    // Vertical line for [+] when folded
                    if (isFoldedLine(blockNumber)) {
                        painter.drawLine(
                            boxX + boxSize / 2, boxY + 2,
                            boxX + boxSize / 2, boxY + boxSize - 2);
                    }
                }
            }
        }

        block = block.next();
        if (!block.isValid()) break;
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void Editor::toggleBookmark(int line)
{
    if (m_bookmarks.contains(line))
        m_bookmarks.remove(line);
    else
        m_bookmarks.insert(line);
    m_lineNumberArea->update();
}

void Editor::nextBookmark()
{
    if (m_bookmarks.isEmpty()) return;
    int current = currentLine();
    QList<int> sorted = m_bookmarks.values();
    std::sort(sorted.begin(), sorted.end());
    for (int bm : sorted) {
        if (bm > current) { goToLine(bm); return; }
    }
    goToLine(sorted.first());
}

void Editor::previousBookmark()
{
    if (m_bookmarks.isEmpty()) return;
    int current = currentLine();
    QList<int> sorted = m_bookmarks.values();
    std::sort(sorted.begin(), sorted.end());
    for (int i = sorted.size() - 1; i >= 0; --i) {
        if (sorted[i] < current) { goToLine(sorted[i]); return; }
    }
    goToLine(sorted.last());
}

void Editor::clearBookmarks()
{
    m_bookmarks.clear();
    m_lineNumberArea->update();
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
    if (findText.isEmpty()) return 0;

    int count = 0;
    QTextCursor cursor(QPlainTextEdit::document());
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::Start);

    while (true) {
        QTextCursor found = QPlainTextEdit::document()->find(findText, cursor, flags);
        if (found.isNull()) break;

        found.insertText(replaceText);
        // Move cursor past the replacement to avoid re-matching
        cursor.setPosition(found.position());
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
    // Column selection: Delete/Backspace deletes selected columns
    if (m_columnSelection.active
        && (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)) {
        deleteColumnSelection();
        return;
    }
    // Column selection: Ctrl+V pastes lines across column rows
    if (m_columnSelection.active
        && event->key() == Qt::Key_V
        && (event->modifiers() & Qt::ControlModifier)) {
        pasteAtColumn();
        return;
    }
    // Column selection: typing inserts at each line
    if (m_columnSelection.active && !event->text().isEmpty() && event->text().at(0).isPrint()) {
        insertTextAtColumn(event->text());
        m_columnSelection.active = false;
        viewport()->update();
        return;
    }
    // Escape cancels column selection
    if (m_columnSelection.active && event->key() == Qt::Key_Escape) {
        m_columnSelection.active = false;
        viewport()->update();
        return;
    }

    // Escape cancels multi-cursor mode
    if (!m_extraCursors.isEmpty() && event->key() == Qt::Key_Escape) {
        clearExtraCursors();
        return;
    }

    // Ctrl+D: select next occurrence
    if (event->key() == Qt::Key_D
        && (event->modifiers() & Qt::ControlModifier)
        && !(event->modifiers() & Qt::ShiftModifier)) {
        selectNextOccurrence();
        return;
    }

    // Ctrl+Shift+L: select all occurrences
    if (event->key() == Qt::Key_L
        && (event->modifiers() & Qt::ControlModifier)
        && (event->modifiers() & Qt::ShiftModifier)) {
        selectAllOccurrences();
        return;
    }

    // Multi-cursor editing: intercept printable keys, backspace, delete, enter
    if (!m_extraCursors.isEmpty()) {
        int key = event->key();
        bool isEditing = (!event->text().isEmpty() && event->text().at(0).isPrint())
            || key == Qt::Key_Backspace
            || key == Qt::Key_Delete
            || key == Qt::Key_Return
            || key == Qt::Key_Enter;

        if (isEditing) {
            applyMultiCursorEdit(event);
            return;
        }
    }

    // Completion popup interaction
    if (m_completionPopup && m_completionPopup->isVisible()) {
        if (event->key() == Qt::Key_Escape) {
            m_completionPopup->hide();
            return;
        }
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            m_completionPopup->acceptCurrent();
            return;
        }
        if (event->key() == Qt::Key_Down) {
            m_completionPopup->selectNext();
            return;
        }
        if (event->key() == Qt::Key_Up) {
            m_completionPopup->selectPrevious();
            return;
        }
        if (event->key() == Qt::Key_Tab) {
            m_completionPopup->acceptCurrent();
            return;
        }
    }

    // Record macro event
    MacroRecorder::instance().recordKeyEvent(event);

    // Auto-close brackets
    if (m_autoCloseBrackets && !event->text().isEmpty()) {
        QChar ch = event->text().at(0);
        QTextCursor cursor = textCursor();
        int pos = cursor.position();
        QString docText = toPlainText();
        QChar nextChar = (pos < docText.length()) ? docText.at(pos) : QChar();

        static const QVector<QPair<QChar, QChar>> bracketPairs = {
            {'(', ')'}, {'[', ']'}, {'{', '}'},
            {'"', '"'}, {'\'', '\''}, {'`', '`'}
        };

        // Skip-over: typing a closing char that matches next char
        for (const auto &pair : bracketPairs) {
            if (ch == pair.second && nextChar == pair.second) {
                cursor.movePosition(QTextCursor::NextCharacter);
                setTextCursor(cursor);
                return;
            }
        }

        // Auto-close: insert closing bracket/quote
        if (shouldAutoClose(cursor)) {
            for (const auto &pair : bracketPairs) {
                if (ch == pair.first) {
                    if (pair.first == pair.second && isInsideString(cursor)) {
                        break;
                    }
                    cursor.beginEditBlock();
                    cursor.insertText(QString(pair.first) + QString(pair.second));
                    cursor.movePosition(QTextCursor::PreviousCharacter);
                    cursor.endEditBlock();
                    setTextCursor(cursor);
                    return;
                }
            }
        }
    }

    // Backspace between empty bracket pair: delete both
    if (m_autoCloseBrackets && event->key() == Qt::Key_Backspace) {
        QTextCursor cursor = textCursor();
        int pos = cursor.position();
        QString docText = toPlainText();
        if (pos > 0 && pos < docText.length()) {
            QChar before = docText.at(pos - 1);
            QChar after = docText.at(pos);
            static const QVector<QPair<QChar, QChar>> bracketPairs = {
                {'(', ')'}, {'[', ']'}, {'{', '}'},
                {'"', '"'}, {'\'', '\''}, {'`', '`'}
            };
            for (const auto &pair : bracketPairs) {
                if (before == pair.first && after == pair.second) {
                    cursor.beginEditBlock();
                    cursor.movePosition(QTextCursor::PreviousCharacter,
                                        QTextCursor::KeepAnchor);
                    cursor.movePosition(QTextCursor::NextCharacter,
                                        QTextCursor::KeepAnchor, 2);
                    cursor.removeSelectedText();
                    cursor.endEditBlock();
                    setTextCursor(cursor);
                    return;
                }
            }
        }
    }

    // Handle tab key — try snippet expansion first
    if (event->key() == Qt::Key_Tab) {
        if (tryExpandSnippet()) {
            return;
        }
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

    // Restart completion timer on word characters (letters, digits, underscore)
    if (!event->text().isEmpty()) {
        QChar ch = event->text().at(0);
        if (ch.isLetterOrNumber() || ch == QLatin1Char('_')) {
            if (m_completionTimer && m_document
                && m_document->fileMode() == Document::SmallFile) {
                m_completionTimer->start();
            }
        } else if (ch == QLatin1Char('/') || ch == QLatin1Char('.')) {
            // Trigger path completion when typing / or . inside quotes
            if (m_completionTimer && m_document
                && m_document->fileMode() == Document::SmallFile) {
                m_completionTimer->start();
            }
        } else {
            // Non-word character: dismiss completion
            if (m_completionPopup && m_completionPopup->isVisible()) {
                m_completionPopup->hide();
            }
            if (m_completionTimer) {
                m_completionTimer->stop();
            }
        }
    }
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
    matchBrackets();
    if (m_rainbowBrackets) {
        updateRainbowBrackets();
    }
}

void Editor::matchBrackets()
{
    m_bracketSelections.clear();

    // Skip bracket matching for medium/large files — toPlainText() is too expensive
    if (m_document && m_document->fileMode() != Document::SmallFile) {
        updateExtraSelections();
        return;
    }

    QTextCursor cursor = textCursor();
    int pos = cursor.position();
    QString text = toPlainText();

    if (text.isEmpty()) {
        updateExtraSelections();
        return;
    }

    auto checkPos = [&](int p) -> bool {
        if (p < 0 || p >= text.length()) return false;
        QChar ch = text.at(p);

        struct BracketPair { QChar open; QChar close; bool forward; };
        QVector<BracketPair> pairs = {
            {'(', ')', true}, {'[', ']', true}, {'{', '}', true},
            {')', '(', false}, {']', '[', false}, {'}', '{', false}
        };

        for (const auto &pair : pairs) {
            if (ch == pair.open) {
                int match = findMatchingBracket(p,
                    pair.forward ? pair.open : pair.close,
                    pair.forward ? pair.close : pair.open,
                    pair.forward);

                Theme theme = ThemeManager::instance().currentTheme();
                QColor bgColor = (match >= 0) ? theme.bracketMatchBackground : theme.bracketErrorBackground;

                QTextEdit::ExtraSelection sel;
                sel.format.setBackground(bgColor);
                sel.cursor = textCursor();
                sel.cursor.setPosition(p);
                sel.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                m_bracketSelections.append(sel);

                if (match >= 0) {
                    QTextEdit::ExtraSelection matchSel;
                    matchSel.format.setBackground(bgColor);
                    matchSel.cursor = textCursor();
                    matchSel.cursor.setPosition(match);
                    matchSel.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                    m_bracketSelections.append(matchSel);
                }
                return true;
            }
        }
        return false;
    };

    if (!checkPos(pos)) {
        checkPos(pos - 1);
    }

    updateExtraSelections();
}

int Editor::findMatchingBracket(int position, QChar open, QChar close, bool forward) const
{
    QString text = toPlainText();
    int depth = 0;
    int i = position;
    int step = forward ? 1 : -1;

    while (i >= 0 && i < text.length()) {
        QChar ch = text.at(i);
        if (ch == open) depth++;
        else if (ch == close) depth--;

        if (depth == 0) return i;
        i += step;
    }

    return -1;
}

bool Editor::shouldAutoClose(const QTextCursor &cursor) const
{
    int pos = cursor.position();
    QString docText = toPlainText();
    if (pos >= docText.length()) return true;

    QChar nextChar = docText.at(pos);
    return nextChar.isSpace()
        || nextChar == ')' || nextChar == ']' || nextChar == '}'
        || nextChar == '\n';
}

bool Editor::isInsideString(const QTextCursor &cursor) const
{
    QString blockText = cursor.block().text();
    int col = cursor.positionInBlock();

    int singleCount = 0;
    int doubleCount = 0;
    int backtickCount = 0;
    for (int i = 0; i < col; ++i) {
        QChar ch = blockText.at(i);
        if (ch == '\'' && (i == 0 || blockText.at(i - 1) != '\\'))
            singleCount++;
        else if (ch == '"' && (i == 0 || blockText.at(i - 1) != '\\'))
            doubleCount++;
        else if (ch == '`' && (i == 0 || blockText.at(i - 1) != '\\'))
            backtickCount++;
    }

    return (singleCount % 2 != 0) || (doubleCount % 2 != 0) || (backtickCount % 2 != 0);
}

void Editor::setAutoCloseBrackets(bool enabled)
{
    m_autoCloseBrackets = enabled;
}

void Editor::setRainbowBrackets(bool enabled)
{
    m_rainbowBrackets = enabled;
    if (enabled) {
        updateRainbowBrackets();
    } else {
        m_rainbowSelections.clear();
        updateExtraSelections();
    }
}

void Editor::updateRainbowBrackets()
{
    m_rainbowSelections.clear();

    if (m_document && m_document->fileMode() != Document::SmallFile) {
        return;
    }

    static const QColor rainbowColors[6] = {
        QColor(255, 0, 0),       // Red
        QColor(255, 165, 0),     // Orange
        QColor(200, 200, 0),     // Yellow
        QColor(0, 180, 0),       // Green
        QColor(0, 100, 255),     // Blue
        QColor(160, 32, 240)     // Purple
    };

    // Walk visible blocks only for performance
    QTextBlock block = firstVisibleBlock();
    QTextBlock lastBlock = block;
    while (lastBlock.isValid() && lastBlock.isVisible()) {
        QTextBlock next = lastBlock.next();
        if (!next.isValid() || !next.isVisible()) break;
        lastBlock = next;
    }

    // Compute depth at first visible block — use cache when possible
    int firstVisibleNum = block.blockNumber();
    int depth = 0;
    if (firstVisibleNum == m_cachedRainbowFirstBlock) {
        depth = m_cachedRainbowDepth;
    } else {
        QTextBlock scanBlock = QPlainTextEdit::document()->begin();
        while (scanBlock.isValid() && scanBlock.blockNumber() < firstVisibleNum) {
            QString text = scanBlock.text();
            for (int i = 0; i < text.length(); ++i) {
                QChar ch = text.at(i);
                if (ch == '(' || ch == '[' || ch == '{') depth++;
                else if (ch == ')' || ch == ']' || ch == '}') depth--;
            }
            scanBlock = scanBlock.next();
        }
        m_cachedRainbowFirstBlock = firstVisibleNum;
        m_cachedRainbowDepth = depth;
    }

    // Now color visible brackets
    while (block.isValid() && block.blockNumber() <= lastBlock.blockNumber()) {
        QString text = block.text();
        int blockPos = block.position();

        for (int i = 0; i < text.length(); ++i) {
            QChar ch = text.at(i);
            bool isBracket = false;
            bool isOpen = false;

            if (ch == '(' || ch == '[' || ch == '{') {
                isBracket = true;
                isOpen = true;
            } else if (ch == ')' || ch == ']' || ch == '}') {
                isBracket = true;
                isOpen = false;
            }

            if (isBracket) {
                if (!isOpen) depth--;
                int colorIdx = (depth >= 0) ? (depth % 6) : 0;

                QTextEdit::ExtraSelection sel;
                sel.format.setForeground(rainbowColors[colorIdx]);
                sel.format.setFontWeight(QFont::Bold);
                sel.cursor = QTextCursor(QPlainTextEdit::document());
                sel.cursor.setPosition(blockPos + i);
                sel.cursor.movePosition(QTextCursor::NextCharacter,
                                        QTextCursor::KeepAnchor);
                m_rainbowSelections.append(sel);

                if (isOpen) depth++;
            }
        }

        block = block.next();
    }
}

void Editor::updateExtraSelections()
{
    QList<QTextEdit::ExtraSelection> allSelections;

    if (!isReadOnly() && Settings::instance().highlightCurrentLine()) {
        QTextEdit::ExtraSelection lineHighlight;
        Theme theme = ThemeManager::instance().currentTheme();
        lineHighlight.format.setBackground(theme.currentLineBackground);
        lineHighlight.format.setProperty(QTextFormat::FullWidthSelection, true);
        lineHighlight.cursor = textCursor();
        lineHighlight.cursor.clearSelection();
        allSelections.append(lineHighlight);
    }

    allSelections.append(m_bracketSelections);
    allSelections.append(m_rainbowSelections);
    allSelections.append(m_markSelections);
    allSelections.append(m_diagnosticSelections);
    allSelections.append(m_multiCursorSelections);

    setExtraSelections(allSelections);
}

void Editor::markAll(const QString &pattern, const SearchEngine::Options &opts)
{
    m_markSelections.clear();

    if (pattern.isEmpty()) {
        updateExtraSelections();
        return;
    }

    SearchEngine engine;
    QVector<SearchResult> results = engine.findAll(toPlainText(), pattern, opts);

    Theme theme = ThemeManager::instance().currentTheme();

    for (const SearchResult &r : results) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(theme.markHighlightColor);

        QTextBlock block = QPlainTextEdit::document()->findBlockByNumber(r.line - 1);
        if (!block.isValid()) continue;

        sel.cursor = QTextCursor(block);
        sel.cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, r.column);
        sel.cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, r.length);
        m_markSelections.append(sel);
    }

    updateExtraSelections();
}

void Editor::clearMarks()
{
    m_markSelections.clear();
    updateExtraSelections();
}

void Editor::jumpToMatchingBracket()
{
    int pos = textCursor().position();
    QString text = toPlainText();

    auto tryJump = [&](int p) -> bool {
        if (p < 0 || p >= text.length()) return false;
        QChar ch = text.at(p);

        struct BracketPair { QChar open; QChar close; bool forward; };
        QVector<BracketPair> pairs = {
            {'(', ')', true}, {'[', ']', true}, {'{', '}', true},
            {')', '(', false}, {']', '[', false}, {'}', '{', false}
        };

        for (const auto &pair : pairs) {
            if (ch == pair.open) {
                int match = findMatchingBracket(p,
                    pair.forward ? pair.open : pair.close,
                    pair.forward ? pair.close : pair.open,
                    pair.forward);
                if (match >= 0) {
                    QTextCursor cursor = textCursor();
                    cursor.setPosition(match + 1);
                    setTextCursor(cursor);
                    return true;
                }
            }
        }
        return false;
    };

    if (!tryJump(pos)) {
        tryJump(pos - 1);
    }
}

void Editor::onTextChanged()
{
    // Kept for compatibility — no longer the primary modification detector.
    // contentsChange signal (connected in constructor) now handles this
    // with proper distinction between content edits and formatting changes.
}

void Editor::syncToDocument()
{
    // Sync editor content to document (called before save)
    m_syncing = true;
    m_document->content()->setText(toPlainText());
    QPlainTextEdit::document()->setModified(false);
    m_syncing = false;
}

void Editor::syncFromDocument()
{
    // Sync document content to editor (called after load/reload)
    m_syncing = true;
    setPlainText(m_document->text());
    QPlainTextEdit::document()->setModified(false);
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
    QString lang = m_document->language().toLower();

    // HTML/XML use block comments
    if (lang == "html" || lang == "xml" || lang == "htm") {
        QTextCursor cursor = textCursor();
        cursor.beginEditBlock();
        if (cursor.hasSelection()) {
            QString selected = cursor.selectedText();
            if (selected.startsWith("<!-- ") && selected.endsWith(" -->")) {
                // Uncomment
                cursor.insertText(selected.mid(5, selected.length() - 9));
            } else {
                // Comment
                cursor.insertText("<!-- " + selected + " -->");
            }
        } else {
            // Comment current line
            cursor.movePosition(QTextCursor::StartOfBlock);
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            QString line = cursor.selectedText();
            if (line.trimmed().startsWith("<!-- ") && line.trimmed().endsWith(" -->")) {
                QString trimmed = line.trimmed();
                cursor.insertText(line.replace(trimmed, trimmed.mid(5, trimmed.length() - 9)));
            } else {
                cursor.insertText("<!-- " + line + " -->");
            }
        }
        cursor.endEditBlock();
        return;
    }

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
    m_foldMarginVisible = enabled;
    updateLineNumberAreaWidth(0);
    m_lineNumberArea->update();
}

static bool usesIndentFolding(const QString &lang)
{
    QString l = lang.toLower();
    return l == "python" || l == "py" || l == "yaml" || l == "yml"
        || l == "coffeescript" || l == "nim" || l == "haskell";
}

static int lineIndentLevel(const QTextBlock &block, int tabWidth)
{
    QString text = block.text();
    int indent = 0;
    for (int i = 0; i < text.length(); ++i) {
        if (text.at(i) == QLatin1Char(' '))
            indent++;
        else if (text.at(i) == QLatin1Char('\t'))
            indent += tabWidth;
        else
            break;
    }
    return indent;
}

static bool isBlankLine(const QTextBlock &block)
{
    return block.text().trimmed().isEmpty();
}

bool Editor::isFoldableLine(int blockNumber) const
{
    if (m_foldedRegions.contains(blockNumber)) return true;

    QTextBlock block = QPlainTextEdit::document()->findBlockByNumber(blockNumber);
    if (!block.isValid()) return false;

    QString lang = m_document ? m_document->language() : QString();

    if (usesIndentFolding(lang)) {
        // Indentation-based: foldable if a non-blank line is followed by
        // a non-blank line with greater indentation
        if (isBlankLine(block)) return false;

        int myIndent = lineIndentLevel(block, m_tabWidth);
        QTextBlock next = block.next();
        while (next.isValid() && isBlankLine(next)) {
            next = next.next();
        }
        if (!next.isValid()) return false;
        return lineIndentLevel(next, m_tabWidth) > myIndent;
    }

    // Brace-based folding
    QString text = block.text().trimmed();
    if (text.endsWith('{')) return true;
    if (text == "{") return true;

    return false;
}

bool Editor::isFoldedLine(int blockNumber) const
{
    return m_foldedRegions.contains(blockNumber);
}

int Editor::findFoldEnd(int blockNumber) const
{
    QTextBlock block =
        QPlainTextEdit::document()->findBlockByNumber(blockNumber);
    if (!block.isValid()) return -1;

    QString lang = m_document ? m_document->language() : QString();

    if (usesIndentFolding(lang)) {
        // Fold ends when a non-blank line returns to the same or lower indent
        int baseIndent = lineIndentLevel(block, m_tabWidth);
        int lastContentBlock = blockNumber;
        QTextBlock next = block.next();
        int currentBlock = blockNumber + 1;

        while (next.isValid()) {
            if (!isBlankLine(next)) {
                if (lineIndentLevel(next, m_tabWidth) <= baseIndent) {
                    break;
                }
                lastContentBlock = currentBlock;
            }
            next = next.next();
            currentBlock++;
        }

        return (lastContentBlock > blockNumber) ? lastContentBlock : -1;
    }

    // Brace-based folding
    int depth = 0;
    int currentBlock = blockNumber;

    while (block.isValid()) {
        QString text = block.text();
        for (int i = 0; i < text.length(); ++i) {
            QChar ch = text.at(i);
            if (ch == '{') depth++;
            else if (ch == '}') {
                depth--;
                if (depth == 0) return currentBlock;
            }
        }
        block = block.next();
        currentBlock++;
    }

    return -1;
}

void Editor::foldAt(int blockNumber)
{
    if (m_foldedRegions.contains(blockNumber)) return;

    int foldEnd = findFoldEnd(blockNumber);
    if (foldEnd <= blockNumber) return;

    // Cache the lines to be folded (blockNumber+1 through foldEnd)
    QStringList cachedLines;
    for (int i = blockNumber + 1; i <= foldEnd; ++i) {
        QTextBlock block =
            QPlainTextEdit::document()->findBlockByNumber(i);
        if (!block.isValid()) break;
        cachedLines.append(block.text());
    }

    if (cachedLines.isEmpty()) return;

    m_foldedRegions[blockNumber] = cachedLines;

    // Remove the cached lines from the document
    bool oldSyncing = m_syncing;
    m_syncing = true;

    QTextBlock startBlock =
        QPlainTextEdit::document()->findBlockByNumber(blockNumber + 1);
    QTextBlock endBlock =
        QPlainTextEdit::document()->findBlockByNumber(
            blockNumber + cachedLines.size());

    if (startBlock.isValid() && endBlock.isValid()) {
        QTextCursor cursor = textCursor();
        // Select from end of header block to end of last folded block
        cursor.setPosition(startBlock.position() - 1);
        cursor.setPosition(
            endBlock.position() + endBlock.length() - 1,
            QTextCursor::KeepAnchor);
        cursor.beginEditBlock();
        cursor.removeSelectedText();
        cursor.endEditBlock();
    }

    // Add fold indicator to the header line
    QTextBlock headerBlock =
        QPlainTextEdit::document()->findBlockByNumber(blockNumber);
    if (headerBlock.isValid()) {
        QTextCursor cursor(headerBlock);
        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.beginEditBlock();
        cursor.insertText(" [...]");
        cursor.endEditBlock();
    }

    m_syncing = oldSyncing;
    m_lineNumberArea->update();
}

void Editor::unfoldAt(int blockNumber)
{
    if (!m_foldedRegions.contains(blockNumber)) return;

    QStringList cachedLines = m_foldedRegions.take(blockNumber);

    bool oldSyncing = m_syncing;
    m_syncing = true;

    QTextBlock headerBlock =
        QPlainTextEdit::document()->findBlockByNumber(blockNumber);
    if (!headerBlock.isValid()) {
        m_syncing = oldSyncing;
        return;
    }

    // Remove [...] indicator from header line
    QString headerText = headerBlock.text();
    if (headerText.endsWith(" [...]")) {
        QTextCursor cursor(headerBlock);
        cursor.movePosition(QTextCursor::EndOfBlock);
        // " [...]" = 6 chars
        cursor.movePosition(
            QTextCursor::Left, QTextCursor::KeepAnchor, 6);
        cursor.beginEditBlock();
        cursor.removeSelectedText();

        // Re-insert cached lines
        cursor.movePosition(QTextCursor::EndOfBlock);
        for (const QString &line : cachedLines) {
            cursor.insertText("\n" + line);
        }
        cursor.endEditBlock();
    }

    m_syncing = oldSyncing;
    m_lineNumberArea->update();
}

void Editor::toggleFoldAt(int blockNumber)
{
    if (m_foldedRegions.contains(blockNumber)) {
        unfoldAt(blockNumber);
    } else if (isFoldableLine(blockNumber)) {
        foldAt(blockNumber);
    }
}

void Editor::foldAll()
{
    for (int i = 0; i < QPlainTextEdit::document()->blockCount(); ++i) {
        if (isFoldableLine(i) && !isFoldedLine(i)) {
            foldAt(i);
        }
    }
}

void Editor::unfoldAll()
{
    QList<int> foldedKeys = m_foldedRegions.keys();
    std::sort(foldedKeys.begin(), foldedKeys.end(),
              std::greater<int>());
    for (int key : foldedKeys) {
        unfoldAt(key);
    }
}

void Editor::toggleFoldAtCursor()
{
    int blockNum = textCursor().blockNumber();
    toggleFoldAt(blockNum);
}

// Whitespace / EOL / indent guide visualization

void Editor::setShowWhitespace(bool show)
{
    m_showWhitespace = show;
    viewport()->update();
}

void Editor::setShowEOL(bool show)
{
    m_showEOL = show;
    viewport()->update();
}

void Editor::setShowIndentGuide(bool show)
{
    m_showIndentGuide = show;
    viewport()->update();
}

// --- Multi-cursor editing ---

void Editor::addCursorAtPosition(int position)
{
    QTextCursor cur(QPlainTextEdit::document());
    cur.setPosition(position);
    m_extraCursors.append(cur);
    updateMultiCursorSelections();
}

void Editor::clearExtraCursors()
{
    m_extraCursors.clear();
    m_multiCursorSelections.clear();
    updateExtraSelections();
    viewport()->update();
}

QString Editor::wordUnderCursor() const
{
    QTextCursor cur = textCursor();
    cur.select(QTextCursor::WordUnderCursor);
    return cur.selectedText();
}

void Editor::selectNextOccurrence()
{
    QString searchText;
    QTextCursor primary = textCursor();

    if (primary.hasSelection()) {
        searchText = primary.selectedText();
    } else {
        // Select the word under cursor at the primary cursor
        primary.select(QTextCursor::WordUnderCursor);
        searchText = primary.selectedText();
        if (searchText.isEmpty()) return;
        setTextCursor(primary);
    }

    // Determine the search start: after the last extra cursor, or after the primary
    int searchFrom = primary.selectionEnd();
    for (const auto &ec : m_extraCursors) {
        int end = ec.selectionEnd();
        if (end > searchFrom) searchFrom = end;
    }

    QTextDocument *doc = QPlainTextEdit::document();
    QTextCursor found = doc->find(searchText, searchFrom);

    // Wrap around if not found
    if (found.isNull()) {
        found = doc->find(searchText, 0);
    }

    if (found.isNull()) return;

    // Skip if this occurrence is already the primary cursor's selection
    if (found.selectionStart() == primary.selectionStart()
        && found.selectionEnd() == primary.selectionEnd()) {
        return;
    }

    // Skip if this occurrence is already covered by an extra cursor
    for (const auto &ec : m_extraCursors) {
        if (found.selectionStart() == ec.selectionStart()
            && found.selectionEnd() == ec.selectionEnd()) {
            return;
        }
    }

    m_extraCursors.append(found);
    updateMultiCursorSelections();
}

void Editor::selectAllOccurrences()
{
    QString searchText;
    QTextCursor primary = textCursor();

    if (primary.hasSelection()) {
        searchText = primary.selectedText();
    } else {
        primary.select(QTextCursor::WordUnderCursor);
        searchText = primary.selectedText();
        if (searchText.isEmpty()) return;
        setTextCursor(primary);
    }

    m_extraCursors.clear();

    QTextDocument *doc = QPlainTextEdit::document();
    QTextCursor found = doc->find(searchText, 0);

    while (!found.isNull()) {
        // Skip the primary cursor's occurrence
        if (found.selectionStart() != primary.selectionStart()
            || found.selectionEnd() != primary.selectionEnd()) {
            m_extraCursors.append(found);
        }
        found = doc->find(searchText, found.selectionEnd());
    }

    updateMultiCursorSelections();
}

void Editor::updateMultiCursorSelections()
{
    m_multiCursorSelections.clear();
    Theme theme = ThemeManager::instance().currentTheme();

    for (const auto &cur : m_extraCursors) {
        if (cur.hasSelection()) {
            // Highlight the selection
            QTextEdit::ExtraSelection sel;
            sel.format.setBackground(theme.selectionBackground.isValid()
                ? theme.selectionBackground : palette().highlight().color());
            sel.format.setForeground(theme.selectionForeground.isValid()
                ? theme.selectionForeground : palette().highlightedText().color());
            sel.cursor = cur;
            m_multiCursorSelections.append(sel);
        }

        // Cursor caret marker (zero-width selection with border)
        QTextEdit::ExtraSelection caretSel;
        caretSel.format.setBackground(theme.foreground.isValid()
            ? theme.foreground : palette().text().color());
        caretSel.format.setProperty(QTextFormat::FullWidthSelection, false);
        caretSel.cursor = cur;
        caretSel.cursor.clearSelection();
        // Select one character forward if possible, to draw a visible caret
        if (!caretSel.cursor.atEnd()) {
            caretSel.cursor.movePosition(QTextCursor::NextCharacter,
                                          QTextCursor::KeepAnchor);
            caretSel.format.setForeground(theme.background.isValid()
                ? theme.background : palette().base().color());
        }
        m_multiCursorSelections.append(caretSel);
    }

    updateExtraSelections();
    viewport()->update();
}

void Editor::applyMultiCursorEdit(QKeyEvent *event)
{
    m_inMultiCursorEdit = true;

    // Collect all cursors: primary + extras
    QVector<QTextCursor> allCursors;
    allCursors.append(textCursor());  // primary is index 0
    allCursors.append(m_extraCursors);

    // Save primary cursor position before sorting
    int primaryPos = textCursor().position();

    // Sort by position descending so edits don't shift earlier positions
    std::sort(allCursors.begin(), allCursors.end(),
        [](const QTextCursor &a, const QTextCursor &b) {
            return a.position() > b.position();
        });

    // Find which index the primary ended up at after sorting
    int primaryIdx = 0;
    for (int i = 0; i < allCursors.size(); ++i) {
        if (allCursors[i].position() == primaryPos) {
            primaryIdx = i;
            break;
        }
    }

    // Bracket pair map for auto-close
    static const QVector<QPair<QChar, QChar>> bracketPairs = {
        {'(', ')'}, {'[', ']'}, {'{', '}'},
        {'"', '"'}, {'\'', '\''}, {'`', '`'}
    };

    QTextCursor editBlock(QPlainTextEdit::document());
    editBlock.beginEditBlock();

    for (auto &cur : allCursors) {
        if (event->key() == Qt::Key_Backspace) {
            if (cur.hasSelection()) {
                cur.removeSelectedText();
            } else {
                cur.deletePreviousChar();
            }
        } else if (event->key() == Qt::Key_Delete) {
            if (cur.hasSelection()) {
                cur.removeSelectedText();
            } else {
                cur.deleteChar();
            }
        } else if (event->key() == Qt::Key_Return
                   || event->key() == Qt::Key_Enter) {
            // Auto-indent: match leading whitespace of current line
            QString blockText = cur.block().text();
            QString indent;
            for (const QChar &ch : blockText) {
                if (ch == ' ' || ch == '\t') indent += ch;
                else break;
            }
            cur.insertText("\n" + indent);
        } else if (!event->text().isEmpty() && event->text().at(0).isPrint()) {
            if (cur.hasSelection()) {
                cur.removeSelectedText();
            }
            // Auto-close brackets in multi-cursor mode
            bool handled = false;
            if (m_autoCloseBrackets) {
                QChar ch = event->text().at(0);
                for (const auto &pair : bracketPairs) {
                    if (ch == pair.first) {
                        if (pair.first == pair.second && isInsideString(cur)) {
                            break;
                        }
                        cur.insertText(QString(pair.first) + QString(pair.second));
                        cur.movePosition(QTextCursor::PreviousCharacter);
                        handled = true;
                        break;
                    }
                }
            }
            if (!handled) {
                cur.insertText(event->text());
            }
        }
    }

    editBlock.endEditBlock();

    // Restore primary cursor correctly
    setTextCursor(allCursors[primaryIdx]);

    // Rebuild extra cursors (everything except primary)
    m_extraCursors.clear();
    for (int i = 0; i < allCursors.size(); ++i) {
        if (i != primaryIdx) {
            m_extraCursors.append(allCursors[i]);
        }
    }

    updateMultiCursorSelections();
    m_inMultiCursorEdit = false;
}

void Editor::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);

    if (m_showWhitespace || m_showEOL || m_showIndentGuide) {
        QPainter painter(viewport());
        Theme theme = ThemeManager::instance().currentTheme();

        if (m_showIndentGuide) {
            paintIndentGuides(painter, theme);
        }
        if (m_showWhitespace) {
            paintWhitespace(painter, theme);
        }
        if (m_showEOL) {
            paintEOL(painter, theme);
        }
    }

    if (m_columnSelection.active) {
        paintColumnSelection();
    }

    // Paint extra cursor carets as 2px-wide rectangles
    if (!m_extraCursors.isEmpty()) {
        QPainter painter(viewport());
        Theme theme = ThemeManager::instance().currentTheme();
        QColor caretColor = theme.foreground.isValid()
            ? theme.foreground : palette().text().color();
        painter.setPen(Qt::NoPen);
        painter.setBrush(caretColor);

        for (const auto &cur : m_extraCursors) {
            QTextBlock block = cur.block();
            if (!block.isVisible()) continue;

            QRectF blockGeom = blockBoundingGeometry(block).translated(contentOffset());
            if (blockGeom.bottom() < 0 || blockGeom.top() > viewport()->height())
                continue;

            QTextLayout *layout = block.layout();
            if (!layout) continue;

            int posInBlock = cur.position() - block.position();
            QTextLine line = layout->lineForTextPosition(posInBlock);
            if (!line.isValid()) continue;

            qreal x = line.cursorToX(posInBlock);
            qreal y = blockGeom.top() + line.y();
            qreal h = line.height();
            painter.drawRect(QRectF(x, y, 2.0, h));
        }
    }
}

void Editor::paintWhitespace(QPainter &painter, const Theme &theme)
{
    painter.setPen(theme.whitespaceColor);

    QTextBlock block = firstVisibleBlock();

    while (block.isValid()) {
        QRectF blockGeom = blockBoundingGeometry(block).translated(contentOffset());
        if (blockGeom.top() > viewport()->rect().bottom())
            break;

        if (block.isVisible() && blockGeom.bottom() >= 0) {
            QTextLayout *layout = block.layout();
            QString text = block.text();

            for (int i = 0; i < text.length(); ++i) {
                QChar ch = text.at(i);
                if (ch == QLatin1Char(' ') || ch == QLatin1Char('\t')) {
                    QTextLine line = layout->lineForTextPosition(i);
                    if (!line.isValid())
                        continue;

                    qreal x = line.cursorToX(i);
                    qreal lineY = blockGeom.top() + line.y();
                    qreal lineH = line.height();

                    if (ch == QLatin1Char(' ')) {
                        qreal dotX = x + fontMetrics().horizontalAdvance(QLatin1Char(' ')) / 2.0;
                        qreal dotY = lineY + lineH / 2.0;
                        painter.drawEllipse(QPointF(dotX, dotY), 1.2, 1.2);
                    } else {
                        qreal nextX = line.cursorToX(i + 1);
                        qreal cy = lineY + lineH / 2.0;
                        painter.drawLine(QPointF(x + 2, cy), QPointF(nextX - 2, cy));
                        painter.drawLine(QPointF(nextX - 5, cy - 3), QPointF(nextX - 2, cy));
                        painter.drawLine(QPointF(nextX - 5, cy + 3), QPointF(nextX - 2, cy));
                    }
                }
            }
        }
        block = block.next();
    }
}

void Editor::paintEOL(QPainter &painter, const Theme &theme)
{
    painter.setPen(theme.whitespaceColor);
    QFont eolFont = font();
    eolFont.setPointSizeF(font().pointSizeF() * 0.7);
    painter.setFont(eolFont);

    QTextBlock block = firstVisibleBlock();

    while (block.isValid()) {
        QRectF blockGeom = blockBoundingGeometry(block).translated(contentOffset());
        if (blockGeom.top() > viewport()->rect().bottom())
            break;

        if (block.isVisible() && blockGeom.bottom() >= 0) {
            QTextLayout *layout = block.layout();
            if (layout->lineCount() > 0) {
                QTextLine lastLine = layout->lineAt(layout->lineCount() - 1);
                qreal x = lastLine.cursorToX(block.text().length());
                qreal y = blockGeom.top() + lastLine.y() + lastLine.ascent();

                painter.drawText(QPointF(x + 4, y), QStringLiteral("\u00B6"));
            }
        }
        block = block.next();
    }

    painter.setFont(font());
}

void Editor::paintIndentGuides(QPainter &painter, const Theme &theme)
{
    QPen pen(theme.indentGuideColor);
    pen.setStyle(Qt::DotLine);
    pen.setWidthF(1.0);
    painter.setPen(pen);

    qreal spaceWidth = fontMetrics().horizontalAdvance(QLatin1Char(' '));
    qreal tabW = spaceWidth * m_tabWidth;

    QTextBlock block = firstVisibleBlock();

    while (block.isValid()) {
        QRectF blockGeom = blockBoundingGeometry(block).translated(contentOffset());
        if (blockGeom.top() > viewport()->rect().bottom())
            break;

        if (block.isVisible() && blockGeom.bottom() >= 0) {
            QString text = block.text();
            int indent = 0;
            for (int i = 0; i < text.length(); ++i) {
                if (text.at(i) == QLatin1Char(' '))
                    indent++;
                else if (text.at(i) == QLatin1Char('\t'))
                    indent += m_tabWidth;
                else
                    break;
            }

            int levels = indent / m_tabWidth;
            for (int level = 1; level <= levels; ++level) {
                qreal x = level * tabW;
                painter.drawLine(QPointF(x, blockGeom.top()),
                                 QPointF(x, blockGeom.bottom()));
            }
        }
        block = block.next();
    }
}

// --- Column/block selection ---

void Editor::mousePressEvent(QMouseEvent *event)
{
    // Ctrl+click: go to definition via LSP
    if (event->modifiers() & Qt::ControlModifier
        && event->button() == Qt::LeftButton) {
        QTextCursor cursor = cursorForPosition(event->pos());
        setTextCursor(cursor);
        requestGotoDefinition();
        return;
    }

    if (event->modifiers() & Qt::AltModifier && event->button() == Qt::LeftButton) {
        // Record Alt+Click position; we decide in mouseReleaseEvent whether
        // this was a click (add cursor) or drag (column selection).
        m_altClickPending = true;
        m_altClickPos = event->pos();

        QTextCursor cursor = cursorForPosition(event->pos());
        m_columnSelection.active = true;
        m_columnSelection.startLine = cursor.blockNumber();
        m_columnSelection.startCol = cursor.columnNumber();
        m_columnSelection.endLine = m_columnSelection.startLine;
        m_columnSelection.endCol = m_columnSelection.startCol;
        viewport()->update();
        return;
    }

    if (m_columnSelection.active) {
        m_columnSelection.active = false;
        viewport()->update();
    }

    // Clicking without Alt clears extra cursors
    if (!m_extraCursors.isEmpty()) {
        clearExtraCursors();
    }

    QPlainTextEdit::mousePressEvent(event);
}

void Editor::mouseMoveEvent(QMouseEvent *event)
{
    if (m_columnSelection.active) {
        QTextCursor cursor = cursorForPosition(event->pos());
        m_columnSelection.endLine = cursor.blockNumber();
        m_columnSelection.endCol = cursor.columnNumber();
        viewport()->update();
        return;
    }
    QPlainTextEdit::mouseMoveEvent(event);
}

void Editor::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_altClickPending && event->button() == Qt::LeftButton) {
        m_altClickPending = false;

        // If the mouse didn't move significantly, treat as Alt+Click (add cursor)
        QPoint delta = event->pos() - m_altClickPos;
        bool wasDrag = (delta.manhattanLength() > 3);

        if (!wasDrag) {
            // Cancel the column selection that was started
            m_columnSelection.active = false;

            // Add a new cursor at the click position
            QTextCursor cursor = cursorForPosition(event->pos());
            addCursorAtPosition(cursor.position());
            return;
        }
        // Otherwise, it was a drag — column selection stays active
        return;
    }

    if (m_columnSelection.active && event->button() == Qt::LeftButton) {
        return;
    }
    QPlainTextEdit::mouseReleaseEvent(event);
}

void Editor::paintColumnSelection()
{
    if (!m_columnSelection.active) return;

    QPainter painter(viewport());
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    int startLine = qMin(m_columnSelection.startLine, m_columnSelection.endLine);
    int endLine = qMax(m_columnSelection.startLine, m_columnSelection.endLine);
    int startCol = qMin(m_columnSelection.startCol, m_columnSelection.endCol);
    int endCol = qMax(m_columnSelection.startCol, m_columnSelection.endCol);

    QColor selColor(0, 120, 215, 80);

    for (int line = startLine; line <= endLine; ++line) {
        QTextBlock block = QPlainTextEdit::document()->findBlockByNumber(line);
        if (!block.isValid() || !block.isVisible()) continue;

        QRectF blockGeom = blockBoundingGeometry(block).translated(contentOffset());
        if (blockGeom.bottom() < 0 || blockGeom.top() > viewport()->height()) continue;

        QTextLayout *layout = block.layout();
        if (layout->lineCount() == 0) continue;
        QTextLine textLine = layout->lineAt(0);

        qreal x1 = textLine.cursorToX(qMin(startCol, block.text().length()));
        qreal x2 = textLine.cursorToX(qMin(endCol, block.text().length()));

        QRectF rect(x1, blockGeom.top(), x2 - x1, blockGeom.height());
        painter.fillRect(rect, selColor);
    }
}

void Editor::insertTextAtColumn(const QString &text)
{
    int startLine = qMin(m_columnSelection.startLine, m_columnSelection.endLine);
    int endLine = qMax(m_columnSelection.startLine, m_columnSelection.endLine);
    int col = qMin(m_columnSelection.startCol, m_columnSelection.endCol);

    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    for (int line = startLine; line <= endLine; ++line) {
        QTextBlock block = QPlainTextEdit::document()->findBlockByNumber(line);
        if (!block.isValid()) continue;

        int pos = block.position() + qMin(col, block.text().length());
        cursor.setPosition(pos);
        cursor.insertText(text);
    }

    cursor.endEditBlock();
    setTextCursor(cursor);
}

void Editor::insertNumbersAtColumn(int initial, int increment, bool leadingZeros)
{
    if (!m_columnSelection.active) return;

    int startLine = qMin(m_columnSelection.startLine, m_columnSelection.endLine);
    int endLine = qMax(m_columnSelection.startLine, m_columnSelection.endLine);
    int col = qMin(m_columnSelection.startCol, m_columnSelection.endCol);

    int maxNum = initial + (endLine - startLine) * increment;
    int width = QString::number(maxNum).length();

    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    int num = initial;
    for (int line = startLine; line <= endLine; ++line) {
        QTextBlock block = QPlainTextEdit::document()->findBlockByNumber(line);
        if (!block.isValid()) continue;

        QString numStr = QString::number(num);
        if (leadingZeros) {
            numStr = numStr.rightJustified(width, '0');
        }

        int pos = block.position() + qMin(col, block.text().length());
        cursor.setPosition(pos);
        cursor.insertText(numStr);
        num += increment;
    }

    cursor.endEditBlock();
    setTextCursor(cursor);
}

void Editor::deleteColumnSelection()
{
    if (!m_columnSelection.active) return;

    int startLine = qMin(m_columnSelection.startLine, m_columnSelection.endLine);
    int endLine = qMax(m_columnSelection.startLine, m_columnSelection.endLine);
    int startCol = qMin(m_columnSelection.startCol, m_columnSelection.endCol);
    int endCol = qMax(m_columnSelection.startCol, m_columnSelection.endCol);

    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    for (int line = endLine; line >= startLine; --line) {
        QTextBlock block = QPlainTextEdit::document()->findBlockByNumber(line);
        if (!block.isValid()) continue;

        int lineLen = block.text().length();
        int colStart = qMin(startCol, lineLen);
        int colEnd = qMin(endCol, lineLen);
        if (colStart >= colEnd) continue;

        int posStart = block.position() + colStart;
        int posEnd = block.position() + colEnd;
        cursor.setPosition(posStart);
        cursor.setPosition(posEnd, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }

    cursor.endEditBlock();
    setTextCursor(cursor);
    m_columnSelection.active = false;
    viewport()->update();
}

void Editor::pasteAtColumn()
{
    if (!m_columnSelection.active) return;

    QString clipText = QApplication::clipboard()->text();
    if (clipText.isEmpty()) return;

    QStringList lines = clipText.split('\n');

    int startLine = qMin(m_columnSelection.startLine, m_columnSelection.endLine);
    int endLine = qMax(m_columnSelection.startLine, m_columnSelection.endLine);
    int col = qMin(m_columnSelection.startCol, m_columnSelection.endCol);

    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    int lineCount = endLine - startLine + 1;
    for (int i = 0; i < lineCount; ++i) {
        QTextBlock block = QPlainTextEdit::document()->findBlockByNumber(startLine + i);
        if (!block.isValid()) continue;

        QString pasteText = (i < lines.size()) ? lines.at(i) : QString();
        int pos = block.position() + qMin(col, block.text().length());
        cursor.setPosition(pos);
        cursor.insertText(pasteText);
    }

    cursor.endEditBlock();
    setTextCursor(cursor);
    m_columnSelection.active = false;
    viewport()->update();
}

void Editor::clearColumnSelection()
{
    m_columnSelection.active = false;
    viewport()->update();
}

// --- LSP Integration ---

void Editor::setDiagnostics(const QVector<Diagnostic> &diagnostics)
{
    m_diagnosticSelections.clear();

    for (const Diagnostic &diag : diagnostics) {
        QTextEdit::ExtraSelection sel;

        QColor underlineColor;
        switch (diag.severity) {
        case 1: underlineColor = Qt::red; break;
        case 2: underlineColor = QColor(255, 165, 0); break;
        case 3: underlineColor = Qt::blue; break;
        case 4: underlineColor = Qt::gray; break;
        default: underlineColor = Qt::red; break;
        }

        sel.format.setUnderlineColor(underlineColor);
        sel.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        sel.format.setToolTip(diag.message);

        QTextBlock startBlock =
            QPlainTextEdit::document()->findBlockByNumber(diag.line);
        QTextBlock endBlock =
            QPlainTextEdit::document()->findBlockByNumber(diag.endLine);

        if (startBlock.isValid() && endBlock.isValid()) {
            sel.cursor = QTextCursor(startBlock);
            sel.cursor.movePosition(QTextCursor::Right,
                                    QTextCursor::MoveAnchor,
                                    diag.character);

            QTextCursor endCursor(endBlock);
            endCursor.movePosition(QTextCursor::Right,
                                   QTextCursor::MoveAnchor,
                                   diag.endCharacter);
            sel.cursor.setPosition(endCursor.position(),
                                   QTextCursor::KeepAnchor);

            m_diagnosticSelections.append(sel);
        }
    }

    updateExtraSelections();
}

void Editor::showLSPCompletions(const QVector<CompletionItem> &items)
{
    if (items.isEmpty() || !m_completionPopup) return;
    m_completionPopup->setCompletions(items);
    QRect rect = cursorRect();
    QPoint pos = mapToGlobal(QPoint(rect.left(), rect.bottom()));
    m_completionPopup->showAtPosition(pos);
}

void Editor::requestHover(int line, int character)
{
    if (!m_document) return;
    QString lang = m_document->language();
    LSPClient *client =
        LSPManager::instance().clientForLanguage(lang);
    if (!client || !client->isInitialized()) return;

    QString uri = QStringLiteral("file://") + m_document->filePath();
    client->hover(uri, line, character);
}

void Editor::requestGotoDefinition()
{
    if (!m_document) return;
    QString lang = m_document->language();
    LSPClient *client =
        LSPManager::instance().clientForLanguage(lang);
    if (!client || !client->isInitialized()) return;

    int line = textCursor().blockNumber();
    int col = textCursor().columnNumber();
    QString uri = QStringLiteral("file://") + m_document->filePath();
    client->gotoDefinition(uri, line, col);
}

void Editor::requestLSPCompletion()
{
    if (!m_document) return;
    QString lang = m_document->language();
    LSPClient *client =
        LSPManager::instance().clientForLanguage(lang);
    if (!client || !client->isInitialized()) return;

    int line = textCursor().blockNumber();
    int col = textCursor().columnNumber();
    QString uri = QStringLiteral("file://") + m_document->filePath();
    client->completion(uri, line, col);
}

void Editor::loadViewportContent()
{
    if (!m_document || !m_document->isLargeFile()) return;
    LargeFileReader *reader = m_document->largeFileReader();
    if (!reader) return;

    // Don't attempt viewport loading until the background index is ready
    if (!reader->isIndexReady()) return;

    qint64 scrollLine = verticalScrollBar()->value();
    qint64 visibleLines = height() / fontMetrics().height();

    // Only reload if near the edge of the current buffer
    qint64 bufferStart = m_viewportStartLine;
    qint64 bufferEnd = m_viewportStartLine + QPlainTextEdit::document()->blockCount();

    if (scrollLine >= bufferStart + 100 &&
        scrollLine + visibleLines <= bufferEnd - 100) {
        return; // Still within comfortable buffer range
    }

    // Calculate new viewport range
    qint64 startLine = qMax(qint64(0), scrollLine - VIEWPORT_BUFFER);
    qint64 endLine = qMin(reader->lineCount(),
                          scrollLine + visibleLines + VIEWPORT_BUFFER);

    m_viewportStartLine = startLine;

    bool oldSyncing = m_syncing;
    m_syncing = true;

    QStringList lines = reader->lines(startLine, endLine - startLine);

    // Block scrollbar signals to prevent re-entrant updates
    verticalScrollBar()->blockSignals(true);
    setPlainText(lines.join('\n'));
    QPlainTextEdit::document()->setModified(false); // viewport swap is not a user edit
    verticalScrollBar()->blockSignals(false);

    // Position cursor relative to viewport
    int localLine = static_cast<int>(scrollLine - startLine);
    QTextBlock block = QPlainTextEdit::document()->findBlockByNumber(localLine);
    if (block.isValid()) {
        QTextCursor cursor = textCursor();
        cursor.setPosition(block.position());
        setTextCursor(cursor);
    }

    m_syncing = oldSyncing;
}

void Editor::triggerCompletion()
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
    QString prefix = cursor.selectedText();

    if (prefix.length() < 2) {
        if (m_completionPopup) {
            m_completionPopup->hide();
        }
        return;
    }

    QVector<SimpleCompletionItem> items = gatherCompletions(prefix);
    if (items.isEmpty()) {
        if (m_completionPopup) {
            m_completionPopup->hide();
        }
        return;
    }

    bool isPathCompletion = !items.isEmpty()
                            && items.first().kind == SimpleCompletionItem::Path;

    if (!m_completionPopup) {
        m_completionPopup = new CompletionPopup(this);
        connect(m_completionPopup, &CompletionPopup::completionSelected,
                this, [this](const QString &completion) {
            QTextCursor cur = textCursor();
            // Check if replacing a path segment (after last /)
            cur.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
            QString selected = cur.selectedText();
            // For path completions, check if there's a partial filename to replace
            int posInBlock = textCursor().positionInBlock();
            QString lineText = textCursor().block().text();
            int lastSlash = lineText.lastIndexOf('/', posInBlock - 1);
            if (lastSlash >= 0 && lastSlash < posInBlock) {
                // Check if the completion looks like a path item
                int blockPos = textCursor().block().position();
                cur.setPosition(blockPos + lastSlash + 1);
                cur.setPosition(blockPos + posInBlock, QTextCursor::KeepAnchor);
            }
            cur.insertText(completion);
        });
    }

    m_completionPopup->setSimpleCompletions(items);
    QRect rect = cursorRect();
    QPoint pos = mapToGlobal(QPoint(rect.left(), rect.bottom()));
    m_completionPopup->showAtPosition(pos);
}

QVector<SimpleCompletionItem> Editor::gatherCompletions(const QString &prefix)
{
    QVector<SimpleCompletionItem> results;
    QSet<QString> seen;

    // Check for path completion: cursor inside quotes with path-like prefix
    QTextCursor cur = textCursor();
    int posInBlock = cur.positionInBlock();
    QString lineText = cur.block().text();
    bool insideQuotes = false;
    QChar quoteChar;
    int quoteStart = -1;
    for (int i = 0; i < posInBlock; ++i) {
        if (lineText[i] == '"' || lineText[i] == '\'') {
            if (!insideQuotes) {
                insideQuotes = true;
                quoteChar = lineText[i];
                quoteStart = i + 1;
            } else if (lineText[i] == quoteChar) {
                insideQuotes = false;
            }
        }
    }
    if (insideQuotes && quoteStart >= 0) {
        QString pathPrefix = lineText.mid(quoteStart, posInBlock - quoteStart);
        if (pathPrefix.startsWith('/') || pathPrefix.startsWith("./")
            || pathPrefix.startsWith("../") || pathPrefix.startsWith("~/")) {
            QVector<SimpleCompletionItem> pathItems = gatherPathCompletions(pathPrefix);
            if (!pathItems.isEmpty()) return pathItems;
        }
    }

    // Gather snippets for current language
    QString lang = language();
    QVector<SimpleCompletionItem> snippets = getSnippetsForLanguage(lang);
    for (const SimpleCompletionItem &snip : snippets) {
        int score = fuzzyMatchScore(snip.label, prefix);
        if (score > 0 && !seen.contains(snip.label)) {
            seen.insert(snip.label);
            results.append(snip);
        }
    }

    // Gather words from the current document with fuzzy matching
    QString text = toPlainText();
    QRegularExpression wordRe("\\b([A-Za-z_]\\w{2,})\\b");
    QRegularExpressionMatchIterator it = wordRe.globalMatch(text);

    struct ScoredItem {
        SimpleCompletionItem item;
        int score;
    };
    QVector<ScoredItem> scored;

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString word = match.captured(1);
        if (word == prefix || seen.contains(word)) continue;

        int score = fuzzyMatchScore(word, prefix);
        if (score > 0) {
            seen.insert(word);
            SimpleCompletionItem item;
            item.label = word;
            item.kind = SimpleCompletionItem::Word;
            scored.append({item, score});
        }
    }

    // Sort by score descending
    std::sort(scored.begin(), scored.end(),
              [](const ScoredItem &a, const ScoredItem &b) {
                  return a.score > b.score;
              });

    for (const ScoredItem &si : scored) {
        results.append(si.item);
        if (results.size() >= 50) break;
    }

    return results;
}

int Editor::fuzzyMatchScore(const QString &candidate, const QString &pattern)
{
    if (pattern.isEmpty()) return 0;
    if (candidate.isEmpty()) return 0;

    int score = 0;
    int patIdx = 0;
    int prevMatchPos = -1;
    bool allMatched = true;
    QString candLower = candidate.toLower();
    QString patLower = pattern.toLower();

    for (int i = 0; i < candLower.length() && patIdx < patLower.length(); ++i) {
        if (candLower[i] == patLower[patIdx]) {
            score += 1;
            // Consecutive match bonus
            if (prevMatchPos == i - 1) {
                score += 3;
            }
            // Word boundary bonus: start, after underscore, camelCase
            if (i == 0) {
                score += 5;
            } else {
                QChar prev = candidate[i - 1];
                if (prev == '_' || prev == '-' || prev == '.') {
                    score += 3;
                } else if (candidate[i].isUpper() && prev.isLower()) {
                    score += 3;
                }
            }
            // Exact case match bonus
            if (candidate[i] == pattern[patIdx]) {
                score += 1;
            }
            prevMatchPos = i;
            ++patIdx;
        }
    }

    // All pattern characters must be found
    if (patIdx < patLower.length()) return 0;

    // Bonus for exact prefix match
    if (candLower.startsWith(patLower)) {
        score += 10;
    }

    return score;
}

QVector<SimpleCompletionItem> Editor::gatherPathCompletions(const QString &prefix)
{
    QVector<SimpleCompletionItem> results;

    QString expandedPath = prefix;
    if (expandedPath.startsWith("~/")) {
        expandedPath = QDir::homePath() + expandedPath.mid(1);
    }

    QFileInfo fi(expandedPath);
    QString dirPath;
    QString filePrefix;

    if (expandedPath.endsWith('/')) {
        dirPath = expandedPath;
        filePrefix = QString();
    } else {
        dirPath = fi.path();
        filePrefix = fi.fileName();
    }

    QDir dir(dirPath);
    if (!dir.exists()) return results;

    QFileInfoList entries = dir.entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot, QDir::DirsFirst | QDir::Name);

    for (const QFileInfo &entry : entries) {
        if (!filePrefix.isEmpty()
            && !entry.fileName().startsWith(filePrefix, Qt::CaseInsensitive)) {
            continue;
        }

        SimpleCompletionItem item;
        if (entry.isDir()) {
            item.label = entry.fileName() + "/";
            item.detail = tr("Directory");
        } else {
            item.label = entry.fileName();
            item.detail = tr("File");
        }
        item.kind = SimpleCompletionItem::Path;
        results.append(item);

        if (results.size() >= 30) break;
    }

    return results;
}

QVector<SimpleCompletionItem> Editor::getSnippetsForLanguage(const QString &lang)
{
    QVector<SimpleCompletionItem> snippets;
    QString lowerLang = lang.toLower();

    auto addSnippet = [&](const QString &trigger, const QString &expansion,
                          const QString &detail) {
        SimpleCompletionItem item;
        item.label = trigger;
        item.detail = detail;
        item.kind = SimpleCompletionItem::Snippet;
        Q_UNUSED(expansion);
        snippets.append(item);
    };

    if (lowerLang == "c++" || lowerLang == "cpp" || lowerLang == "c") {
        addSnippet("for", "for (int i = 0; i < count; ++i) {\n\t\n}",
                   "for loop");
        addSnippet("if", "if (condition) {\n\t\n}",
                   "if statement");
        addSnippet("while", "while (condition) {\n\t\n}",
                   "while loop");
        addSnippet("class", "class Name {\npublic:\n\tName();\n\t~Name();\n\nprivate:\n\t\n};",
                   "class template");
        addSnippet("switch", "switch (expr) {\ncase value:\n\tbreak;\ndefault:\n\tbreak;\n}",
                   "switch statement");
        addSnippet("main", "int main(int argc, char *argv[]) {\n\t\n\treturn 0;\n}",
                   "main function");
    } else if (lowerLang == "python") {
        addSnippet("def", "def function_name(args):\n\t\"\"\"Docstring.\"\"\"\n\tpass",
                   "function");
        addSnippet("class", "class ClassName:\n\tdef __init__(self):\n\t\tpass",
                   "class template");
        addSnippet("for", "for item in iterable:\n\tpass",
                   "for loop");
        addSnippet("with", "with open(filename, 'r') as f:\n\tpass",
                   "with statement");
        addSnippet("try", "try:\n\tpass\nexcept Exception as e:\n\tpass",
                   "try/except");
        addSnippet("ifmain", "if __name__ == '__main__':\n\tmain()",
                   "if main guard");
    } else if (lowerLang == "javascript" || lowerLang == "js") {
        addSnippet("fn", "function name() {\n\t\n}",
                   "function");
        addSnippet("afn", "const name = (args) => {\n\t\n};",
                   "arrow function");
        addSnippet("for", "for (let i = 0; i < count; i++) {\n\t\n}",
                   "for loop");
        addSnippet("foreach", "array.forEach((item) => {\n\t\n});",
                   "forEach");
        addSnippet("class", "class Name {\n\tconstructor() {\n\t\t\n\t}\n}",
                   "class template");
        addSnippet("try", "try {\n\t\n} catch (error) {\n\t\n}",
                   "try/catch");
    } else if (lowerLang == "typescript" || lowerLang == "ts") {
        addSnippet("fn", "function name(): void {\n\t\n}",
                   "function");
        addSnippet("afn", "const name = (args: type): ReturnType => {\n\t\n};",
                   "arrow function");
        addSnippet("interface", "interface Name {\n\t\n}",
                   "interface");
        addSnippet("class", "class Name {\n\tconstructor() {\n\t\t\n\t}\n}",
                   "class template");
        addSnippet("for", "for (let i = 0; i < count; i++) {\n\t\n}",
                   "for loop");
        addSnippet("try", "try {\n\t\n} catch (error: unknown) {\n\t\n}",
                   "try/catch");
    } else if (lowerLang == "rust") {
        addSnippet("fn", "fn name() {\n\t\n}",
                   "function");
        addSnippet("struct", "struct Name {\n\t\n}",
                   "struct");
        addSnippet("impl", "impl Name {\n\t\n}",
                   "impl block");
        addSnippet("match", "match value {\n\t_ => {},\n}",
                   "match expression");
    } else if (lowerLang == "go") {
        addSnippet("fn", "func name() {\n\t\n}",
                   "function");
        addSnippet("struct", "type Name struct {\n\t\n}",
                   "struct");
        addSnippet("for", "for i := 0; i < count; i++ {\n\t\n}",
                   "for loop");
        addSnippet("iferr", "if err != nil {\n\treturn err\n}",
                   "error check");
    } else if (lowerLang == "java") {
        addSnippet("class", "public class Name {\n\t\n}",
                   "class");
        addSnippet("main", "public static void main(String[] args) {\n\t\n}",
                   "main method");
        addSnippet("for", "for (int i = 0; i < count; i++) {\n\t\n}",
                   "for loop");
        addSnippet("try", "try {\n\t\n} catch (Exception e) {\n\t\n}",
                   "try/catch");
    }

    return snippets;
}

bool Editor::tryExpandSnippet()
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
    QString word = cursor.selectedText();
    if (word.isEmpty()) return false;

    QString lang = language().toLower();
    QVector<SimpleCompletionItem> snippets = getSnippetsForLanguage(lang);

    // Build a map of trigger -> expansion
    // We need the actual expansion text, so rebuild with a direct lookup
    QMap<QString, QString> expansionMap;

    if (lang == "c++" || lang == "cpp" || lang == "c") {
        expansionMap["for"] = "for (int i = 0; i < count; ++i) {\n\t\n}";
        expansionMap["if"] = "if (condition) {\n\t\n}";
        expansionMap["while"] = "while (condition) {\n\t\n}";
        expansionMap["class"] = "class Name {\npublic:\n\tName();\n\t~Name();\n\nprivate:\n\t\n};";
        expansionMap["switch"] = "switch (expr) {\ncase value:\n\tbreak;\ndefault:\n\tbreak;\n}";
        expansionMap["main"] = "int main(int argc, char *argv[]) {\n\t\n\treturn 0;\n}";
    } else if (lang == "python") {
        expansionMap["def"] = "def function_name(args):\n\t\"\"\"Docstring.\"\"\"\n\tpass";
        expansionMap["class"] = "class ClassName:\n\tdef __init__(self):\n\t\tpass";
        expansionMap["for"] = "for item in iterable:\n\tpass";
        expansionMap["with"] = "with open(filename, 'r') as f:\n\tpass";
        expansionMap["try"] = "try:\n\tpass\nexcept Exception as e:\n\tpass";
        expansionMap["ifmain"] = "if __name__ == '__main__':\n\tmain()";
    } else if (lang == "javascript" || lang == "js") {
        expansionMap["fn"] = "function name() {\n\t\n}";
        expansionMap["afn"] = "const name = (args) => {\n\t\n};";
        expansionMap["for"] = "for (let i = 0; i < count; i++) {\n\t\n}";
        expansionMap["foreach"] = "array.forEach((item) => {\n\t\n});";
        expansionMap["class"] = "class Name {\n\tconstructor() {\n\t\t\n\t}\n}";
        expansionMap["try"] = "try {\n\t\n} catch (error) {\n\t\n}";
    } else if (lang == "typescript" || lang == "ts") {
        expansionMap["fn"] = "function name(): void {\n\t\n}";
        expansionMap["afn"] = "const name = (args: type): ReturnType => {\n\t\n};";
        expansionMap["interface"] = "interface Name {\n\t\n}";
        expansionMap["class"] = "class Name {\n\tconstructor() {\n\t\t\n\t}\n}";
        expansionMap["for"] = "for (let i = 0; i < count; i++) {\n\t\n}";
        expansionMap["try"] = "try {\n\t\n} catch (error: unknown) {\n\t\n}";
    } else if (lang == "rust") {
        expansionMap["fn"] = "fn name() {\n\t\n}";
        expansionMap["struct"] = "struct Name {\n\t\n}";
        expansionMap["impl"] = "impl Name {\n\t\n}";
        expansionMap["match"] = "match value {\n\t_ => {},\n}";
    } else if (lang == "go") {
        expansionMap["fn"] = "func name() {\n\t\n}";
        expansionMap["struct"] = "type Name struct {\n\t\n}";
        expansionMap["for"] = "for i := 0; i < count; i++ {\n\t\n}";
        expansionMap["iferr"] = "if err != nil {\n\treturn err\n}";
    } else if (lang == "java") {
        expansionMap["class"] = "public class Name {\n\t\n}";
        expansionMap["main"] = "public static void main(String[] args) {\n\t\n}";
        expansionMap["for"] = "for (int i = 0; i < count; i++) {\n\t\n}";
        expansionMap["try"] = "try {\n\t\n} catch (Exception e) {\n\t\n}";
    }

    if (!expansionMap.contains(word)) return false;

    QString expansion = expansionMap[word];

    // Replace tabs with the configured indent string
    QString indentStr = indentString();
    expansion.replace("\t", indentStr);

    // Get current line indent for multi-line snippets
    QString currentIndent;
    QString blockText = cursor.block().text();
    for (const QChar &ch : blockText) {
        if (ch == ' ' || ch == '\t') {
            currentIndent += ch;
        } else {
            break;
        }
    }

    // Add current indent to each new line in the expansion
    expansion.replace("\n", "\n" + currentIndent);

    cursor.beginEditBlock();
    cursor.insertText(expansion);
    cursor.endEditBlock();
    setTextCursor(cursor);

    // Hide completion popup if visible
    if (m_completionPopup && m_completionPopup->isVisible()) {
        m_completionPopup->hide();
    }

    return true;
}

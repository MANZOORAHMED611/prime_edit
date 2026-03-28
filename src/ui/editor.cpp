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
#include <algorithm>

Editor::Editor(Document *document, QWidget *parent)
    : QPlainTextEdit(parent)
    , m_document(document)
{
    setupEditor();
    applySettings();

    // Create line number area
    m_lineNumberArea = new LineNumberArea(this);

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
        m_bookmarkMarginVisible = false;
        m_foldMarginVisible = false;
        m_lineNumbersVisible = false;
        QPlainTextEdit::document()->setUndoRedoEnabled(false);
    } else if (m_document->fileMode() == Document::MediumFile) {
        QPlainTextEdit::document()->setUndoRedoEnabled(true);
    }

    // Initialize
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    // Load content
    m_syncing = true;
    if (m_document->fileMode() == Document::LargeFile) {
        // Large file: load directly into QTextDocument — skip PieceTable entirely
        // This avoids: readAll→QString→PieceTable→setPlainText (4 copies, ~650MB for 93MB file)
        // Instead: read in chunks → append to QTextDocument (1 copy)
        setUpdatesEnabled(false);
        QTextCursor cursor(QPlainTextEdit::document());
        QFile file(m_document->filePath());
        if (file.open(QIODevice::ReadOnly)) {
            while (!file.atEnd()) {
                QByteArray chunk = file.read(2 * 1024 * 1024); // 2MB chunks
                cursor.insertText(QString::fromUtf8(chunk));
            }
            file.close();
        }
        setUpdatesEnabled(true);
    } else if (!m_document->text().isEmpty()) {
        setPlainText(m_document->text());
    }
    m_syncing = false;

    // Set language — but skip syntax highlighting for medium/large files
    if (!m_document->language().isEmpty() && m_document->fileMode() == Document::SmallFile) {
        m_highlighter->setLanguage(m_document->language());
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
    if (m_bookmarkMarginVisible) {
        QRect bmRect(0, event->rect().top(), bmWidth, event->rect().height());
        painter.fillRect(bmRect, theme.bookmarkMarginBackground);
    }
    if (m_lineNumbersVisible) {
        QRect lnRect(bmWidth, event->rect().top(), lineNumWidth, event->rect().height());
        painter.fillRect(lnRect, theme.lineNumberBackground);
    }
    if (m_foldMarginVisible) {
        QRect fmRect(bmWidth + lineNumWidth, event->rect().top(), fmWidth, event->rect().height());
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

            // Section 1: Bookmark indicators
            if (m_bookmarkMarginVisible && m_bookmarks.contains(blockNumber + 1)) {
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setBrush(QColor(0x33, 0x99, 0xFF));
                painter.setPen(Qt::NoPen);
                int cx = bmWidth / 2;
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
                painter.drawText(bmWidth, top, lineNumWidth - 5, fontMetrics().height(),
                                Qt::AlignRight, number);
            }

            // Section 3: Fold margin indicators
            if (m_foldMarginVisible) {
                int foldX = bmWidth + lineNumWidth;
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
    if (findText.isEmpty()) {
        return 0;
    }

    int count = 0;
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::Start);
    setTextCursor(cursor);

    while (findNext(findText, flags)) {
        QTextCursor current = textCursor(); // get the cursor with selection from findNext
        current.insertText(replaceText);
        ++count;
    }

    cursor = textCursor();
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
    matchBrackets();
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
    allSelections.append(m_markSelections);
    allSelections.append(m_diagnosticSelections);

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

bool Editor::isFoldableLine(int blockNumber) const
{
    if (m_foldedRegions.contains(blockNumber)) return true;

    QTextBlock block = QPlainTextEdit::document()->findBlockByNumber(blockNumber);
    if (!block.isValid()) return false;

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

    if (!m_completionPopup) {
        m_completionPopup = new CompletionPopup(this);
        connect(m_completionPopup, &CompletionPopup::completionSelected,
                this, [this](const QString &completion) {
            QTextCursor cur = textCursor();
            cur.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
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

    // Gather words from the current document
    QString text = toPlainText();
    QRegularExpression wordRe("\\b([A-Za-z_]\\w{2,})\\b");
    QRegularExpressionMatchIterator it = wordRe.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString word = match.captured(1);
        if (word.startsWith(prefix, Qt::CaseInsensitive) && word != prefix && !seen.contains(word)) {
            seen.insert(word);
            SimpleCompletionItem item;
            item.label = word;
            item.kind = SimpleCompletionItem::Word;
            results.append(item);
        }
    }

    return results;
}

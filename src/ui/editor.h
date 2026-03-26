#ifndef EDITOR_H
#define EDITOR_H

#include <QPlainTextEdit>
#include <QTextDocument>
#include <QMap>
#include <QSet>
#include <QTimer>

#include "core/searchengine.h"
#include "ui/completionpopup.h"

class QPainter;
struct Theme;
class Document;
class SyntaxHighlighter;
class LineNumberArea;

class Editor : public QPlainTextEdit
{
    Q_OBJECT

public:
    static constexpr int BOOKMARK_MARGIN_WIDTH = 16;
    static constexpr int FOLD_MARGIN_WIDTH = 16;

    explicit Editor(Document *document, QWidget *parent = nullptr);
    ~Editor() override;

    Document *document() const { return m_document; }

    // Line numbers
    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void setLineNumbersVisible(bool visible);
    bool lineNumbersVisible() const { return m_lineNumbersVisible; }

    // Bookmarks
    void toggleBookmark(int line);
    void nextBookmark();
    void previousBookmark();
    void clearBookmarks();
    QSet<int> bookmarks() const { return m_bookmarks; }

    // Gutter section widths
    int bookmarkMarginWidth() const;
    int foldMarginWidth() const;

    // Expose protected QPlainTextEdit methods for LineNumberArea
    using QPlainTextEdit::firstVisibleBlock;
    using QPlainTextEdit::blockBoundingGeometry;
    using QPlainTextEdit::blockBoundingRect;
    using QPlainTextEdit::contentOffset;

    // Settings
    void setWordWrapEnabled(bool enabled);
    bool wordWrapEnabled() const;
    void setTabWidth(int width);
    int tabWidth() const { return m_tabWidth; }
    void setInsertSpaces(bool insert);
    bool insertSpaces() const { return m_insertSpaces; }

    // Zoom
    void zoomIn(int range = 1);
    void zoomOut(int range = 1);
    void resetZoom();

    // Navigation
    void goToLine(int line);
    int currentLine() const;
    int currentColumn() const;

    // Search
    bool findNext(const QString &text, QTextDocument::FindFlags flags = QTextDocument::FindFlags());
    bool findPrevious(const QString &text, QTextDocument::FindFlags flags = QTextDocument::FindFlags());
    int replaceAll(const QString &findText, const QString &replaceText,
                   QTextDocument::FindFlags flags = QTextDocument::FindFlags());

    // Mark system
    void markAll(const QString &pattern, const SearchEngine::Options &opts);
    void clearMarks();
    int markCount() const { return m_markSelections.size(); }

    // Selection
    QString selectedText() const;
    void replaceSelection(const QString &text);

    // Column/block selection
    bool isColumnSelectionActive() const { return m_columnSelection.active; }
    void clearColumnSelection();

    // Language
    void setLanguage(const QString &language);
    QString language() const;

    // Sync
    void syncToDocument();
    void syncFromDocument();

    // Line operations
    void duplicateLine();
    void deleteLine();
    void moveLineUp();
    void moveLineDown();
    void toggleComment();
    void sortLinesAscending();
    void sortLinesDescending();
    void removeDuplicateLines();
    void trimTrailingWhitespace();
    void joinLines();
    void toUpperCase();
    void toLowerCase();
    void toTitleCase();

    // Code folding
    void setFoldingEnabled(bool enabled);
    bool foldingEnabled() const { return m_foldingEnabled; }
    void foldAll();
    void unfoldAll();
    void toggleFoldAtCursor();
    void foldAt(int blockNumber);
    void unfoldAt(int blockNumber);
    void toggleFoldAt(int blockNumber);
    bool isFoldableLine(int blockNumber) const;
    bool isFoldedLine(int blockNumber) const;
    QMap<int, QStringList> foldedRegions() const { return m_foldedRegions; }

    // Whitespace / EOL / indent guide visualization
    void setShowWhitespace(bool show);
    bool showWhitespace() const { return m_showWhitespace; }
    void setShowEOL(bool show);
    bool showEOL() const { return m_showEOL; }
    void setShowIndentGuide(bool show);
    bool showIndentGuide() const { return m_showIndentGuide; }

public slots:
    void jumpToMatchingBracket();

signals:
    void cursorPositionChanged();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();
    void onTextChanged();

private:
    void setupEditor();
    void applySettings();
    QString indentString() const;

    Document *m_document;
    SyntaxHighlighter *m_highlighter;
    LineNumberArea *m_lineNumberArea;

    QSet<int> m_bookmarks;
    bool m_bookmarkMarginVisible = true;
    bool m_foldMarginVisible = true;

    int m_tabWidth = 4;
    bool m_insertSpaces = true;
    bool m_lineNumbersVisible = true;
    bool m_foldingEnabled = true;
    int m_zoomLevel = 0;
    int m_baseFontSize;

    bool m_syncing = false;

    // Code folding
    QMap<int, QStringList> m_foldedRegions;
    int findFoldEnd(int blockNumber) const;

    // Column/block selection
    struct ColumnSelection {
        int startLine = -1, startCol = -1;
        int endLine = -1, endCol = -1;
        bool active = false;
    };
    ColumnSelection m_columnSelection;
    void paintColumnSelection();
    void insertTextAtColumn(const QString &text);

    // Whitespace / EOL / indent guide visualization
    bool m_showWhitespace = false;
    bool m_showEOL = false;
    bool m_showIndentGuide = false;
    void paintWhitespace(QPainter &painter, const Theme &theme);
    void paintEOL(QPainter &painter, const Theme &theme);
    void paintIndentGuides(QPainter &painter, const Theme &theme);

    // Bracket matching
    void matchBrackets();
    int findMatchingBracket(int position, QChar open, QChar close, bool forward) const;
    void updateExtraSelections();
    QList<QTextEdit::ExtraSelection> m_bracketSelections;
    QList<QTextEdit::ExtraSelection> m_markSelections;

    QString getCommentString() const;

    // Auto-completion
    void triggerCompletion();
    QVector<SimpleCompletionItem> gatherCompletions(const QString &prefix);
    QTimer *m_completionTimer = nullptr;
    CompletionPopup *m_completionPopup = nullptr;
    int m_consecutiveWordChars = 0;
};

#endif // EDITOR_H

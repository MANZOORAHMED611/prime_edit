#ifndef EDITOR_H
#define EDITOR_H

#include <QPlainTextEdit>
#include <QTextDocument>

class Document;
class SyntaxHighlighter;
class LineNumberArea;

class Editor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit Editor(Document *document, QWidget *parent = nullptr);
    ~Editor() override;

    Document *document() const { return m_document; }

    // Line numbers
    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void setLineNumbersVisible(bool visible);
    bool lineNumbersVisible() const { return m_lineNumbersVisible; }

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

    // Selection
    QString selectedText() const;
    void replaceSelection(const QString &text);

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

signals:
    void cursorPositionChanged();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

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

    int m_tabWidth = 4;
    bool m_insertSpaces = true;
    bool m_lineNumbersVisible = true;
    bool m_foldingEnabled = true;
    int m_zoomLevel = 0;
    int m_baseFontSize;

    bool m_syncing = false;

    QString getCommentString() const;
};

#endif // EDITOR_H

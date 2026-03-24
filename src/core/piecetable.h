#ifndef PIECETABLE_H
#define PIECETABLE_H

#include <QString>
#include <QVector>
#include <memory>

// Piece table data structure for efficient text editing of large files
// Supports efficient insertions, deletions, and undo/redo operations

struct Piece {
    enum Source { Original, Add };
    Source source;
    qint64 start;
    qint64 length;
};

class PieceTable {
public:
    PieceTable();
    explicit PieceTable(const QString &text);

    // Basic operations
    void insert(qint64 position, const QString &text);
    void remove(qint64 position, qint64 length);
    QString text() const;
    QString textRange(qint64 start, qint64 length) const;
    qint64 length() const;

    // Line operations
    qint64 lineCount() const;
    qint64 lineStart(qint64 lineNumber) const;
    qint64 lineEnd(qint64 lineNumber) const;
    qint64 lineLength(qint64 lineNumber) const;
    QString line(qint64 lineNumber) const;
    qint64 lineForPosition(qint64 position) const;
    qint64 columnForPosition(qint64 position) const;

    // Undo/Redo
    struct UndoEntry {
        enum Type { Insert, Remove };
        Type type;
        qint64 position;
        QString text;
    };

    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();
    void clearUndoHistory();
    void beginUndoGroup();
    void endUndoGroup();

    // Reset
    void clear();
    void setText(const QString &text);

private:
    QString m_originalBuffer;
    QString m_addBuffer;
    QVector<Piece> m_pieces;

    QVector<UndoEntry> m_undoStack;
    QVector<UndoEntry> m_redoStack;
    int m_undoGroupDepth = 0;
    QVector<UndoEntry> m_currentGroup;

    mutable qint64 m_cachedLength = -1;
    mutable QVector<qint64> m_lineStarts;
    mutable bool m_lineStartsValid = false;

    void invalidateCache();
    void rebuildLineCache() const;
    qint64 pieceIndexForPosition(qint64 position, qint64 &offsetInPiece) const;
    QChar charAt(const Piece &piece, qint64 offset) const;
};

#endif // PIECETABLE_H

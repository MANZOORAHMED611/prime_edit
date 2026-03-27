#include "piecetable.h"

PieceTable::PieceTable()
{
}

PieceTable::PieceTable(const QString &text)
    : m_originalBuffer(text)
{
    if (!text.isEmpty()) {
        Piece piece;
        piece.source = Piece::Original;
        piece.start = 0;
        piece.length = text.length();
        m_pieces.append(piece);
    }
    invalidateCache();
}

void PieceTable::insert(qint64 position, const QString &text)
{
    if (text.isEmpty()) return;

    // Record for undo
    UndoEntry entry;
    entry.type = UndoEntry::Insert;
    entry.position = position;
    entry.text = text;

    if (m_undoGroupDepth > 0) {
        m_currentGroup.append(entry);
    } else {
        m_undoStack.append(entry);
        m_redoStack.clear();
    }

    // Add text to add buffer
    qint64 addStart = m_addBuffer.length();
    m_addBuffer.append(text);

    // Create new piece
    Piece newPiece;
    newPiece.source = Piece::Add;
    newPiece.start = addStart;
    newPiece.length = text.length();

    if (m_pieces.isEmpty()) {
        m_pieces.append(newPiece);
    } else {
        // Find piece to split
        qint64 offsetInPiece;
        qint64 pieceIndex = pieceIndexForPosition(position, offsetInPiece);

        if (pieceIndex < 0) {
            // Append at end
            m_pieces.append(newPiece);
        } else if (offsetInPiece == 0) {
            // Insert before piece
            m_pieces.insert(pieceIndex, newPiece);
        } else if (offsetInPiece == m_pieces[pieceIndex].length) {
            // Insert after piece
            m_pieces.insert(pieceIndex + 1, newPiece);
        } else {
            // Split piece
            Piece &original = m_pieces[pieceIndex];
            Piece second;
            second.source = original.source;
            second.start = original.start + offsetInPiece;
            second.length = original.length - offsetInPiece;
            original.length = offsetInPiece;

            m_pieces.insert(pieceIndex + 1, newPiece);
            m_pieces.insert(pieceIndex + 2, second);
        }
    }

    invalidateCache();
}

void PieceTable::remove(qint64 position, qint64 length)
{
    if (length <= 0) return;

    // Record removed text for undo
    UndoEntry entry;
    entry.type = UndoEntry::Remove;
    entry.position = position;
    entry.text = textRange(position, length);

    if (m_undoGroupDepth > 0) {
        m_currentGroup.append(entry);
    } else {
        m_undoStack.append(entry);
        m_redoStack.clear();
    }

    qint64 remaining = length;
    qint64 pos = position;

    while (remaining > 0 && !m_pieces.isEmpty()) {
        qint64 offsetInPiece;
        qint64 pieceIndex = pieceIndexForPosition(pos, offsetInPiece);

        if (pieceIndex < 0) break;

        Piece &piece = m_pieces[pieceIndex];
        qint64 removeFromPiece = qMin(remaining, piece.length - offsetInPiece);

        if (offsetInPiece == 0 && removeFromPiece == piece.length) {
            // Remove entire piece
            m_pieces.remove(pieceIndex);
        } else if (offsetInPiece == 0) {
            // Remove from start
            piece.start += removeFromPiece;
            piece.length -= removeFromPiece;
        } else if (offsetInPiece + removeFromPiece == piece.length) {
            // Remove from end
            piece.length -= removeFromPiece;
        } else {
            // Remove from middle - split
            Piece second;
            second.source = piece.source;
            second.start = piece.start + offsetInPiece + removeFromPiece;
            second.length = piece.length - offsetInPiece - removeFromPiece;
            piece.length = offsetInPiece;
            m_pieces.insert(pieceIndex + 1, second);
        }

        remaining -= removeFromPiece;
    }

    invalidateCache();
}

QString PieceTable::text() const
{
    QString result;
    result.reserve(length());

    for (const Piece &piece : m_pieces) {
        const QString &buffer = (piece.source == Piece::Original) ? m_originalBuffer : m_addBuffer;
        result.append(buffer.mid(piece.start, piece.length));
    }

    return result;
}

QString PieceTable::textRange(qint64 start, qint64 len) const
{
    QString result;
    qint64 remaining = len;
    qint64 pos = start;

    while (remaining > 0) {
        qint64 offsetInPiece;
        qint64 pieceIndex = pieceIndexForPosition(pos, offsetInPiece);

        if (pieceIndex < 0) break;

        const Piece &piece = m_pieces[pieceIndex];
        const QString &buffer = (piece.source == Piece::Original) ? m_originalBuffer : m_addBuffer;

        qint64 copyLen = qMin(remaining, piece.length - offsetInPiece);
        result.append(buffer.mid(piece.start + offsetInPiece, copyLen));

        pos += copyLen;
        remaining -= copyLen;
    }

    return result;
}

qint64 PieceTable::length() const
{
    if (m_cachedLength < 0) {
        m_cachedLength = 0;
        for (const Piece &piece : m_pieces) {
            m_cachedLength += piece.length;
        }
    }
    return m_cachedLength;
}

qint64 PieceTable::lineCount() const
{
    rebuildLineCache();
    return m_lineStarts.size();
}

qint64 PieceTable::lineStart(qint64 lineNumber) const
{
    rebuildLineCache();
    if (lineNumber < 0 || lineNumber >= m_lineStarts.size()) {
        return -1;
    }
    return m_lineStarts[lineNumber];
}

qint64 PieceTable::lineEnd(qint64 lineNumber) const
{
    rebuildLineCache();
    if (lineNumber < 0 || lineNumber >= m_lineStarts.size()) {
        return -1;
    }

    if (lineNumber + 1 < m_lineStarts.size()) {
        qint64 nextStart = m_lineStarts[lineNumber + 1];
        // Exclude the newline character
        return nextStart - 1;
    }
    return length();
}

qint64 PieceTable::lineLength(qint64 lineNumber) const
{
    qint64 start = lineStart(lineNumber);
    qint64 end = lineEnd(lineNumber);
    if (start < 0 || end < 0) return 0;
    return end - start;
}

QString PieceTable::line(qint64 lineNumber) const
{
    qint64 start = lineStart(lineNumber);
    qint64 len = lineLength(lineNumber);
    if (start < 0 || len < 0) return QString();
    return textRange(start, len);
}

qint64 PieceTable::lineForPosition(qint64 position) const
{
    rebuildLineCache();

    // Binary search for line
    qint64 low = 0;
    qint64 high = m_lineStarts.size() - 1;

    while (low <= high) {
        qint64 mid = (low + high) / 2;
        if (m_lineStarts[mid] <= position) {
            if (mid + 1 >= m_lineStarts.size() || m_lineStarts[mid + 1] > position) {
                return mid;
            }
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    return 0;
}

qint64 PieceTable::columnForPosition(qint64 position) const
{
    qint64 line = lineForPosition(position);
    qint64 lineStartPos = lineStart(line);
    return position - lineStartPos;
}

bool PieceTable::canUndo() const
{
    return !m_undoStack.isEmpty();
}

bool PieceTable::canRedo() const
{
    return !m_redoStack.isEmpty();
}

void PieceTable::undo()
{
    if (m_undoStack.isEmpty()) return;

    int gid = m_undoStack.last().groupId;

    // Collect all entries in this group (or just the single entry)
    QVector<UndoEntry> toUndo;
    if (gid > 0) {
        while (!m_undoStack.isEmpty() && m_undoStack.last().groupId == gid) {
            toUndo.prepend(m_undoStack.takeLast());
        }
    } else {
        toUndo.append(m_undoStack.takeLast());
    }

    // Undo in reverse order using internal methods (no undo recording)
    for (int i = toUndo.size() - 1; i >= 0; --i) {
        const UndoEntry &entry = toUndo[i];
        if (entry.type == UndoEntry::Insert) {
            removeInternal(entry.position, entry.text.length());
        } else {
            insertInternal(entry.position, entry.text);
        }
    }

    // Push all entries to redo stack (in original order)
    for (const UndoEntry &entry : toUndo) {
        m_redoStack.append(entry);
    }
}

void PieceTable::redo()
{
    if (m_redoStack.isEmpty()) return;

    int gid = m_redoStack.last().groupId;

    // Collect all entries in this group (or just the single entry)
    QVector<UndoEntry> toRedo;
    if (gid > 0) {
        while (!m_redoStack.isEmpty() && m_redoStack.last().groupId == gid) {
            toRedo.prepend(m_redoStack.takeLast());
        }
    } else {
        toRedo.append(m_redoStack.takeLast());
    }

    // Redo in forward order using internal methods (no undo recording)
    for (const UndoEntry &entry : toRedo) {
        if (entry.type == UndoEntry::Insert) {
            insertInternal(entry.position, entry.text);
        } else {
            removeInternal(entry.position, entry.text.length());
        }
    }

    // Push all entries to undo stack
    for (const UndoEntry &entry : toRedo) {
        m_undoStack.append(entry);
    }
}

void PieceTable::clearUndoHistory()
{
    m_undoStack.clear();
    m_redoStack.clear();
}

void PieceTable::beginUndoGroup()
{
    m_undoGroupDepth++;
}

void PieceTable::endUndoGroup()
{
    if (m_undoGroupDepth <= 0) return;
    m_undoGroupDepth--;
    if (m_undoGroupDepth == 0 && !m_currentGroup.isEmpty()) {
        int gid = m_nextGroupId++;
        for (auto &entry : m_currentGroup) {
            entry.groupId = gid;
            m_undoStack.append(entry);
        }
        m_currentGroup.clear();
        m_redoStack.clear();
    }
}

void PieceTable::insertInternal(qint64 position, const QString &text)
{
    if (text.isEmpty()) return;

    // Add text to add buffer
    qint64 addStart = m_addBuffer.length();
    m_addBuffer.append(text);

    // Create new piece
    Piece newPiece;
    newPiece.source = Piece::Add;
    newPiece.start = addStart;
    newPiece.length = text.length();

    if (m_pieces.isEmpty()) {
        m_pieces.append(newPiece);
    } else {
        qint64 offsetInPiece;
        qint64 pieceIndex = pieceIndexForPosition(position, offsetInPiece);

        if (pieceIndex < 0) {
            m_pieces.append(newPiece);
        } else if (offsetInPiece == 0) {
            m_pieces.insert(pieceIndex, newPiece);
        } else if (offsetInPiece == m_pieces[pieceIndex].length) {
            m_pieces.insert(pieceIndex + 1, newPiece);
        } else {
            Piece &original = m_pieces[pieceIndex];
            Piece second;
            second.source = original.source;
            second.start = original.start + offsetInPiece;
            second.length = original.length - offsetInPiece;
            original.length = offsetInPiece;

            m_pieces.insert(pieceIndex + 1, newPiece);
            m_pieces.insert(pieceIndex + 2, second);
        }
    }

    invalidateCache();
}

void PieceTable::removeInternal(qint64 position, qint64 length)
{
    if (length <= 0) return;

    qint64 remaining = length;
    qint64 pos = position;

    while (remaining > 0 && !m_pieces.isEmpty()) {
        qint64 offsetInPiece;
        qint64 pieceIndex = pieceIndexForPosition(pos, offsetInPiece);

        if (pieceIndex < 0) break;

        Piece &piece = m_pieces[pieceIndex];
        qint64 removeFromPiece = qMin(remaining, piece.length - offsetInPiece);

        if (offsetInPiece == 0 && removeFromPiece == piece.length) {
            m_pieces.remove(pieceIndex);
        } else if (offsetInPiece == 0) {
            piece.start += removeFromPiece;
            piece.length -= removeFromPiece;
        } else if (offsetInPiece + removeFromPiece == piece.length) {
            piece.length -= removeFromPiece;
        } else {
            Piece second;
            second.source = piece.source;
            second.start = piece.start + offsetInPiece + removeFromPiece;
            second.length = piece.length - offsetInPiece - removeFromPiece;
            piece.length = offsetInPiece;
            m_pieces.insert(pieceIndex + 1, second);
        }

        remaining -= removeFromPiece;
    }

    invalidateCache();
}

void PieceTable::clear()
{
    m_originalBuffer.clear();
    m_addBuffer.clear();
    m_pieces.clear();
    m_undoStack.clear();
    m_redoStack.clear();
    invalidateCache();
}

void PieceTable::setText(const QString &text)
{
    clear();
    m_originalBuffer = text;
    if (!text.isEmpty()) {
        Piece piece;
        piece.source = Piece::Original;
        piece.start = 0;
        piece.length = text.length();
        m_pieces.append(piece);
    }
    invalidateCache();
}

void PieceTable::invalidateCache()
{
    m_cachedLength = -1;
    m_lineStartsValid = false;
}

void PieceTable::rebuildLineCache() const
{
    if (m_lineStartsValid) return;

    m_lineStarts.clear();
    m_lineStarts.append(0);  // First line always starts at 0

    qint64 pos = 0;
    for (const Piece &piece : m_pieces) {
        const QString &buffer = (piece.source == Piece::Original) ? m_originalBuffer : m_addBuffer;
        for (qint64 i = 0; i < piece.length; ++i) {
            QChar ch = buffer[piece.start + i];
            if (ch == '\n') {
                m_lineStarts.append(pos + i + 1);
            }
        }
        pos += piece.length;
    }

    m_lineStartsValid = true;
}

qint64 PieceTable::pieceIndexForPosition(qint64 position, qint64 &offsetInPiece) const
{
    qint64 pos = 0;
    for (int i = 0; i < m_pieces.size(); ++i) {
        const Piece &piece = m_pieces[i];
        if (position >= pos && position < pos + piece.length) {
            offsetInPiece = position - pos;
            return i;
        }
        pos += piece.length;
    }

    // Position at end
    if (position == pos && !m_pieces.isEmpty()) {
        offsetInPiece = m_pieces.last().length;
        return m_pieces.size() - 1;
    }

    offsetInPiece = 0;
    return -1;
}

QChar PieceTable::charAt(const Piece &piece, qint64 offset) const
{
    const QString &buffer = (piece.source == Piece::Original) ? m_originalBuffer : m_addBuffer;
    return buffer[piece.start + offset];
}

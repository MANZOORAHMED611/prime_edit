// Spike gate 4(a): prove SCN_MODIFIED carries enough to drive an incremental
// LSP didChange, a git gutter and a dirty flag.
//
// The tracker turns each insert/delete notification into an LSP
// TextDocumentContentChangeEvent (range in line + UTF-16 code units, new
// text) and applies it to a shadow QString, the copy an LSP server would hold.
// If the shadow equals the real buffer after any edit sequence, the
// notifications are sufficient.
#pragma once

#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

#include "ScintillaEdit.h"

namespace demo {

struct LspChange {
    int startLine = 0;
    int startChar = 0; // UTF-16 code units, as LSP's default positionEncoding
    int endLine = 0;
    int endChar = 0;
    QString text;
    QString toJson() const;
};

class ChangeTracker : public QObject
{
    Q_OBJECT
public:
    explicit ChangeTracker(ScintillaEdit *editor);

    // Start from the editor's current text (didOpen).
    void reset();
    const QString &shadow() const { return m_shadow; }
    bool shadowMatchesBuffer() const;
    const QList<LspChange> &changes() const { return m_changes; }
    const QSet<int> &gutterLines() const { return m_gutterLines; }
    bool dirty() const { return m_dirty; }
    const QStringList &dirtyLog() const { return m_dirtyLog; }

private:
    void onNotify(Scintilla::NotificationData *scn);
    void onBeforeDelete(Scintilla::Position pos, Scintilla::Position len);
    void onInsert(Scintilla::Position pos, const char *text, Scintilla::Position len,
                  Scintilla::Position linesAdded);
    void onDelete(Scintilla::Position pos);
    void markGutter(int firstLine, int lastLine);
    void applyToShadow(const LspChange &change);
    // Scintilla positions are UTF-8 byte offsets; LSP wants line + UTF-16 units.
    void toLspPosition(Scintilla::Position pos, int *line, int *character) const;

    ScintillaEdit *m_ed;
    QString m_shadow;
    QList<LspChange> m_changes;
    LspChange m_pendingDelete; // range captured in SC_MOD_BEFOREDELETE
    QSet<int> m_gutterLines;
    bool m_dirty = false;
    QStringList m_dirtyLog;
};

} // namespace demo

#include "changetracker.h"

#include "Scintilla.h"
#include "demosetup.h"

namespace demo {

using Scintilla::ModificationFlags;
using Scintilla::Notification;
using Scintilla::Position;

QString LspChange::toJson() const
{
    QString escaped = text;
    escaped.replace('\\', "\\\\").replace('"', "\\\"").replace('\n', "\\n");
    return QString("{\"range\":{\"start\":{\"line\":%1,\"character\":%2},"
                   "\"end\":{\"line\":%3,\"character\":%4}},\"text\":\"%5\"}")
        .arg(startLine).arg(startChar).arg(endLine).arg(endChar).arg(escaped);
}

ChangeTracker::ChangeTracker(ScintillaEdit *editor)
    : QObject(editor), m_ed(editor)
{
    // BEFOREDELETE is not in the default mask: it is the only moment the
    // deleted range can still be converted to line/column coordinates.
    m_ed->send(SCI_SETMODEVENTMASK, SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT
                                        | SC_MOD_BEFOREDELETE | SC_PERFORMED_USER
                                        | SC_PERFORMED_UNDO | SC_PERFORMED_REDO);
    connect(m_ed, &ScintillaEditBase::notify, this, &ChangeTracker::onNotify);
    connect(m_ed, &ScintillaEditBase::savePointChanged, this, [this](bool dirty) {
        m_dirty = dirty;
        m_dirtyLog << (dirty ? "SCN_SAVEPOINTLEFT" : "SCN_SAVEPOINTREACHED");
    });
    reset();
}

void ChangeTracker::reset()
{
    m_shadow = QString::fromUtf8(bufferText(m_ed));
    m_changes.clear();
    m_gutterLines.clear();
    m_dirtyLog.clear();
}

bool ChangeTracker::shadowMatchesBuffer() const
{
    return m_shadow == QString::fromUtf8(bufferText(m_ed));
}

void ChangeTracker::onNotify(Scintilla::NotificationData *scn)
{
    if (scn->nmhdr.code != Notification::Modified)
        return;
    const int flags = static_cast<int>(scn->modificationType);
    if (flags & SC_MOD_BEFOREDELETE)
        onBeforeDelete(scn->position, scn->length);
    else if (flags & SC_MOD_INSERTTEXT)
        onInsert(scn->position, scn->text, scn->length, scn->linesAdded);
    else if (flags & SC_MOD_DELETETEXT)
        onDelete(scn->position);
}

void ChangeTracker::toLspPosition(Position pos, int *line, int *character) const
{
    const sptr_t l = m_ed->send(SCI_LINEFROMPOSITION, pos);
    const sptr_t lineStart = m_ed->send(SCI_POSITIONFROMLINE, l);
    *line = static_cast<int>(l);
    *character = static_cast<int>(m_ed->send(SCI_COUNTCODEUNITS, lineStart, pos));
}

void ChangeTracker::onBeforeDelete(Position pos, Position len)
{
    m_pendingDelete = LspChange{};
    toLspPosition(pos, &m_pendingDelete.startLine, &m_pendingDelete.startChar);
    toLspPosition(pos + len, &m_pendingDelete.endLine, &m_pendingDelete.endChar);
}

void ChangeTracker::onInsert(Position pos, const char *text, Position len, Position linesAdded)
{
    // Called after the insert: pos is still the start of the inserted text.
    LspChange change;
    toLspPosition(pos, &change.startLine, &change.startChar);
    change.endLine = change.startLine;
    change.endChar = change.startChar;
    change.text = QString::fromUtf8(text, static_cast<int>(len));
    applyToShadow(change);
    markGutter(change.startLine, change.startLine + static_cast<int>(linesAdded));
}

void ChangeTracker::onDelete(Position pos)
{
    applyToShadow(m_pendingDelete);
    const int line = static_cast<int>(m_ed->send(SCI_LINEFROMPOSITION, pos));
    markGutter(line, line);
}

void ChangeTracker::markGutter(int firstLine, int lastLine)
{
    // Scintilla moves markers with their lines on insert/delete, so the gutter
    // never has to be re-indexed by hand when lines shift.
    for (int l = firstLine; l <= lastLine; ++l) {
        m_ed->send(SCI_MARKERADD, l, kGutterMarker);
        m_gutterLines.insert(l);
    }
}

void ChangeTracker::applyToShadow(const LspChange &change)
{
    // What an LSP server does with an incremental change.
    auto offsetOf = [this](int line, int character) {
        int offset = 0;
        for (int l = 0; l < line; ++l)
            offset = m_shadow.indexOf('\n', offset) + 1;
        return offset + character;
    };
    const int start = offsetOf(change.startLine, change.startChar);
    const int end = offsetOf(change.endLine, change.endChar);
    m_shadow.replace(start, end - start, change.text);
    m_changes << change;
}

} // namespace demo

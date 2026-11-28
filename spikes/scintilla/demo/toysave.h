// Spike gate 4(b): save straight from Scintilla's gap buffer, no full copy.
#pragma once

#include <QString>

#include "ScintillaEdit.h"

namespace demo {

struct ToySaveReport {
    bool ok = false;
    QString error;
    qint64 bytes = 0;
    qint64 rssBeforeKb = 0;     // VmRSS before SCI_GETCHARACTERPOINTER
    qint64 rssAfterKb = 0;      // VmRSS after the save committed
    qint64 hwmBeforeKb = 0;     // VmHWM (peak) before
    qint64 hwmAfterKb = 0;      // VmHWM (peak) after
    double pointerMs = 0;       // cost of SCI_GETCHARACTERPOINTER (gap move)
    double writeMs = 0;         // write + fdatasync + commit
};

// Same contract as Document::writeAtomic (unbuffered QSaveFile, checked write,
// checked fdatasync, commit), fed by QByteArray::fromRawData over
// SCI_GETCHARACTERPOINTER, so the buffer is never duplicated.
ToySaveReport toySaveWhole(ScintillaEdit *ed, const QString &path);
// Variant that never moves the gap to the end: writes the document in chunks
// through SCI_GETRANGEPOINTER.
ToySaveReport toySaveChunked(ScintillaEdit *ed, const QString &path, qint64 chunkBytes);

// Reads a "VmRSS:" style field from /proc/self/status, in kB.
qint64 procStatusKb(const char *field);

} // namespace demo

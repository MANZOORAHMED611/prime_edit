#include "toysave.h"

#include <QElapsedTimer>
#include <QFile>
#include <QSaveFile>

#include <cerrno>
#include <unistd.h>

#include "Scintilla.h"

namespace demo {

qint64 procStatusKb(const char *field)
{
    QFile status("/proc/self/status");
    if (!status.open(QIODevice::ReadOnly))
        return -1;
    const QByteArray key(field);
    for (const QByteArray &line : status.readAll().split('\n')) {
        if (line.startsWith(key))
            return line.mid(key.size()).trimmed().split(' ').value(0).toLongLong();
    }
    return -1;
}

namespace {

// Opens like Document::writeAtomic: unbuffered, so a short write is visible.
bool openSave(QSaveFile &file, ToySaveReport &r)
{
    if (file.open(QIODevice::WriteOnly | QIODevice::Unbuffered))
        return true;
    r.error = file.errorString();
    return false;
}

bool finishSave(QSaveFile &file, ToySaveReport &r)
{
    errno = 0;
    if (::fdatasync(file.handle()) != 0) {
        r.error = qt_error_string(errno);
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        r.error = file.errorString();
        return false;
    }
    return true;
}

void snapshotBefore(ToySaveReport &r)
{
    r.rssBeforeKb = procStatusKb("VmRSS:");
    r.hwmBeforeKb = procStatusKb("VmHWM:");
}

void snapshotAfter(ToySaveReport &r)
{
    r.rssAfterKb = procStatusKb("VmRSS:");
    r.hwmAfterKb = procStatusKb("VmHWM:");
}

} // namespace

ToySaveReport toySaveWhole(ScintillaEdit *ed, const QString &path)
{
    ToySaveReport r;
    snapshotBefore(r);
    QElapsedTimer t;
    t.start();
    const char *data = reinterpret_cast<const char *>(ed->send(SCI_GETCHARACTERPOINTER));
    r.bytes = ed->send(SCI_GETLENGTH);
    r.pointerMs = t.nsecsElapsed() / 1e6;
    // fromRawData wraps the pointer: no allocation, no copy. This is the
    // QByteArray that Document::writeAtomic(path, data, mode) would receive.
    const QByteArray view = QByteArray::fromRawData(data, r.bytes);
    QSaveFile file(path);
    if (!openSave(file, r))
        return r;
    t.restart();
    if (file.write(view) != view.size()) {
        r.error = qt_error_string(errno);
        file.cancelWriting();
        return r;
    }
    r.ok = finishSave(file, r);
    r.writeMs = t.nsecsElapsed() / 1e6;
    snapshotAfter(r);
    return r;
}

ToySaveReport toySaveChunked(ScintillaEdit *ed, const QString &path, qint64 chunkBytes)
{
    ToySaveReport r;
    snapshotBefore(r);
    r.bytes = ed->send(SCI_GETLENGTH);
    QSaveFile file(path);
    if (!openSave(file, r))
        return r;
    QElapsedTimer t;
    t.start();
    for (qint64 pos = 0; pos < r.bytes; pos += chunkBytes) {
        const qint64 len = qMin(chunkBytes, r.bytes - pos);
        // Only a chunk that straddles the gap moves it; others are direct.
        const char *p = reinterpret_cast<const char *>(ed->send(SCI_GETRANGEPOINTER, pos, len));
        if (file.write(p, len) != len) {
            r.error = qt_error_string(errno);
            file.cancelWriting();
            return r;
        }
    }
    r.ok = finishSave(file, r);
    r.writeMs = t.nsecsElapsed() / 1e6;
    snapshotAfter(r);
    return r;
}

} // namespace demo

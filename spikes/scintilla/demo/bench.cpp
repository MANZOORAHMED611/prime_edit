#include "bench.h"

#include <QEventLoop>
#include <QFile>
#include <QTimer>

#include <cstdio>
#include <ctime>

#include "ILoader.h"
#include "Scintilla.h"
#include "toysave.h"

namespace demo {
namespace {

qint64 realtimeNs()
{
    timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);
    return qint64(ts.tv_sec) * 1000000000 + ts.tv_nsec;
}

qint64 launchNs()
{
    static const qint64 t0 = [] {
        const qint64 env = qEnvironmentVariable("BENCH_T0_NS").toLongLong();
        return env > 0 ? env : realtimeNs();
    }();
    return t0;
}

double sinceLaunchMs()
{
    return (realtimeNs() - launchNs()) / 1e6;
}

void emitValue(const char *key, double value)
{
    std::printf("BENCH %s=%.1f\n", key, value);
    std::fflush(stdout);
}

// Blocks until Scintilla has finished a paintEvent (ScintillaEditBase::painted).
bool waitForPaint(ScintillaEdit *ed, int timeoutMs)
{
    QEventLoop loop;
    bool painted = false;
    QObject::connect(ed, &ScintillaEditBase::painted, &loop, [&] {
        painted = true;
        loop.quit();
    });
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    ed->viewport()->update();
    loop.exec();
    return painted;
}

} // namespace

bool loadLargeFile(ScintillaEdit *ed, const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        std::fprintf(stderr, "cannot open %s: %s\n", qPrintable(path),
                     qPrintable(file.errorString()));
        return false;
    }
    // BENCH_DOC_OPTIONS lets a run add SC_DOCUMENTOPTION_STYLES_NONE (0x1).
    const int extra = qEnvironmentVariableIntValue("BENCH_DOC_OPTIONS");
    auto *loader = reinterpret_cast<Scintilla::ILoader *>(
        ed->send(SCI_CREATELOADER, file.size() + 1024, SC_DOCUMENTOPTION_TEXT_LARGE | extra));
    QByteArray chunk(16 * 1024 * 1024, Qt::Uninitialized);
    for (qint64 n; (n = file.read(chunk.data(), chunk.size())) > 0;) {
        if (loader->AddData(chunk.constData(), n) != SC_STATUS_OK) {
            loader->Release();
            std::fprintf(stderr, "loader AddData failed\n");
            return false;
        }
    }
    void *doc = loader->ConvertToDocument();
    ed->send(SCI_SETDOCPOINTER, 0, reinterpret_cast<sptr_t>(doc));
    ed->send(SCI_RELEASEDOCUMENT, 0, reinterpret_cast<sptr_t>(doc)); // editor holds the ref
    // BENCH_LAYOUT_THREADS: parallel text measurement for very long lines.
    if (const int threads = qEnvironmentVariableIntValue("BENCH_LAYOUT_THREADS"))
        ed->send(SCI_SETLAYOUTTHREADS, threads);
    return true;
}

int runBench(ScintillaEdit *ed, const QString &path)
{
    launchNs();
    emitValue("t_event_loop_ms", sinceLaunchMs());
    if (!loadLargeFile(ed, path))
        return 1;
    emitValue("t_loaded_ms", sinceLaunchMs());
    emitValue("t_visible_ms", waitForPaint(ed, 600000) ? sinceLaunchMs() : -1);

    const sptr_t before = ed->send(SCI_GETLENGTH);
    ed->send(SCI_INSERTTEXT, 0, reinterpret_cast<sptr_t>("X"));
    const bool edited = ed->send(SCI_GETLENGTH) == before + 1 && waitForPaint(ed, 600000);
    emitValue("t_editable_ms", edited ? sinceLaunchMs() : -1);
    ed->send(SCI_UNDO);

    const double scrollStart = sinceLaunchMs();
    ed->send(SCI_DOCUMENTEND);
    const bool scrolled = waitForPaint(ed, 600000);
    emitValue("scroll_to_end_ms", scrolled ? sinceLaunchMs() - scrollStart : -1);
    emitValue("first_visible_line_after_end", ed->send(SCI_GETFIRSTVISIBLELINE));
    emitValue("line_count", ed->send(SCI_GETLINECOUNT));
    emitValue("bytes", before);
    emitValue("vm_hwm_kb", procStatusKb("VmHWM:"));
    emitValue("vm_rss_kb", procStatusKb("VmRSS:"));
    return 0;
}

} // namespace demo

// Driver for the P1.12 sync-failure receipt (spec
// docs/superpowers/specs/2026-11-28-atomic-short-write.md, section 5 case 5).
// A failed fdatasync cannot be produced on ext4 from CTest, so the receipt runs
// this driver under fdatasync_eio_shim.so (SHIM_SYNC_EIO=1):
//
//   sync_failure_driver doc <existing-file>   Document::save of new text
//   sync_failure_driver raw <existing-file>   FileUtils::writeFileAtomic
//
// Prints the helper's result, lastError() for doc, and whether the file still
// holds its original bytes. Exit 0 when the helper failed and the file is
// unchanged, 1 otherwise. Run with QT_QPA_PLATFORM=offscreen. Not built by
// default: cmake --build <dir> --target sync_failure_driver
#include "test_isolation.h"

#include "core/document.h"
#include "utils/fileutils.h"

#include <QApplication>
#include <QFile>

#include <cerrno>
#include <cstdio>
#include <cstring>

namespace {

QByteArray readAll(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

bool runDocument(const QString &path)
{
    Document doc;
    if (!doc.load(path))
        return false;
    doc.setText(QStringLiteral("new content that must not reach the disk\n"));
    const SaveResult result = doc.save();
    std::printf("Document::save result=%d (IoError=%d) lastError=[%s]\n", int(result),
                int(SaveResult::IoError), qPrintable(doc.lastError()));
    return result == SaveResult::IoError;
}

bool runRaw(const QString &path)
{
    const bool ok = FileUtils::writeFileAtomic(path, QByteArray("new raw bytes\n"));
    std::printf("FileUtils::writeFileAtomic returned=%s\n", ok ? "true" : "false");
    return !ok;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    isolateFromUserData();
    const QStringList args = app.arguments();
    if (args.size() != 3 || (args[1] != QLatin1String("doc") && args[1] != QLatin1String("raw"))) {
        std::fprintf(stderr, "usage: sync_failure_driver doc|raw <existing-file>\n");
        return 2;
    }
    const QString path = args[2];
    const QByteArray before = readAll(path);
    const bool failed = args[1] == QLatin1String("doc") ? runDocument(path) : runRaw(path);
    const bool unchanged = readAll(path) == before;
    std::printf("file unchanged=%s strerror(EIO)=[%s]\n", unchanged ? "yes" : "no",
                std::strerror(EIO));
    return failed && unchanged ? 0 : 1;
}

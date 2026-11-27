#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <QString>
#include <QStringList>

namespace FileUtils {

// File information
QString fileName(const QString &path);
QString fileExtension(const QString &path);
QString fileBaseName(const QString &path);
QString directory(const QString &path);

// File type detection
QString mimeType(const QString &path);
bool isBinary(const QString &path);
bool isTextFile(const QString &path);

// Path utilities
QString absolutePath(const QString &path);
QString relativePath(const QString &path, const QString &base);
QString joinPath(const QString &base, const QString &relative);
bool exists(const QString &path);
bool isReadable(const QString &path);
bool isWritable(const QString &path);

// File filters
QString allFilesFilter();
QString textFilesFilter();
QString sourceFilesFilter();
QString combinedFilter();

// Backup
QString createBackup(const QString &path);
bool restoreBackup(const QString &backupPath, const QString &originalPath);

// Persistence files (spec D10)
// Writes through QSaveFile: the previous content survives any failed write.
bool writeFileAtomic(const QString &path, const QByteArray &bytes);
// Copies an unparsable file to "<path>.corrupt-<yyyyMMdd-hhmmss>" and leaves
// the original in place. Returns the copy's path, or empty if it failed.
QString preserveCorruptCopy(const QString &path);
// fsync on the directory itself, so renames inside it reach the disk.
bool fsyncDirectory(const QString &path);

}

#endif // FILEUTILS_H

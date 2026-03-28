#include "fileutils.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QMimeDatabase>
#include <QDateTime>

namespace FileUtils {

QString fileName(const QString &path)
{
    return QFileInfo(path).fileName();
}

QString fileExtension(const QString &path)
{
    return QFileInfo(path).suffix();
}

QString fileBaseName(const QString &path)
{
    return QFileInfo(path).baseName();
}

QString directory(const QString &path)
{
    return QFileInfo(path).absolutePath();
}

QString mimeType(const QString &path)
{
    QMimeDatabase db;
    return db.mimeTypeForFile(path).name();
}

bool isBinary(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    // Read first 8KB and check for null bytes
    QByteArray data = file.read(8192);
    file.close();

    return data.contains('\0');
}

bool isTextFile(const QString &path)
{
    QString mime = mimeType(path);
    return mime.startsWith("text/") || !isBinary(path);
}

QString absolutePath(const QString &path)
{
    return QFileInfo(path).absoluteFilePath();
}

QString relativePath(const QString &path, const QString &base)
{
    QDir baseDir(base);
    return baseDir.relativeFilePath(path);
}

QString joinPath(const QString &base, const QString &relative)
{
    return QDir(base).filePath(relative);
}

bool exists(const QString &path)
{
    return QFileInfo::exists(path);
}

bool isReadable(const QString &path)
{
    return QFileInfo(path).isReadable();
}

bool isWritable(const QString &path)
{
    QFileInfo info(path);
    if (info.exists()) {
        return info.isWritable();
    }
    // Check if directory is writable
    return QFileInfo(info.absolutePath()).isWritable();
}

QString allFilesFilter()
{
    return QObject::tr("All Files (*)");
}

QString textFilesFilter()
{
    return QObject::tr("Text Files (*.txt *.text *.log *.md *.markdown *.rst *.csv *.json *.xml *.yaml *.yml *.toml *.ini *.cfg *.conf)");
}

QString sourceFilesFilter()
{
    return QObject::tr("Source Files (*.cpp *.c *.h *.hpp *.cc *.cxx *.py *.js *.ts *.jsx *.tsx *.java *.rs *.go *.rb *.php *.cs *.swift *.kt *.scala *.dart *.lua *.sh *.bash *.sql *.html *.css *.scss)");
}

QString combinedFilter()
{
    return allFilesFilter() + ";;" + textFilesFilter() + ";;" + sourceFilesFilter();
}

QString createBackup(const QString &path)
{
    if (!exists(path)) {
        return QString();
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString backupPath = path + "." + timestamp + ".bak";

    if (QFile::copy(path, backupPath)) {
        return backupPath;
    }
    return QString();
}

bool restoreBackup(const QString &backupPath, const QString &originalPath)
{
    if (!exists(backupPath)) {
        return false;
    }

    if (exists(originalPath)) {
        QFile::remove(originalPath);
    }

    return QFile::copy(backupPath, originalPath);
}

}

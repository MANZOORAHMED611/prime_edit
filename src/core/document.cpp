#include "document.h"
#include "charsetdetector.h"
#include "largefile.h"
#include "utils/settings.h"
#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QStringEncoder>
#include <QStringDecoder>
#include <QSaveFile>

#include <sys/stat.h>
#include <unistd.h>

Document::Document(QObject *parent)
    : QObject(parent)
    , m_uuid(QUuid::createUuid().toString(QUuid::Id128))
{
}

Document::~Document()
{
}

bool Document::load(const QString &filePath)
{
    QFileInfo fi(filePath);
    qint64 fileSize = fi.size();

    // Determine file mode
    if (fileSize > LARGE_FILE_THRESHOLD) {
        m_fileMode = LargeFile;
    } else if (fileSize > MEDIUM_FILE_THRESHOLD) {
        // Check if file is minified (very few newlines) — treat as Large
        QFile probe(filePath);
        if (probe.open(QIODevice::ReadOnly)) {
            QByteArray sample = probe.read(100000); // check first 100KB
            probe.close();
            int newlines = sample.count('\n');
            // If less than 10 newlines in 100KB, it's minified → Large mode
            if (newlines < 10 && fileSize > 5 * 1024 * 1024) {
                m_isMinified = true;
                m_fileMode = LargeFile;
            } else {
                m_fileMode = MediumFile;
            }
        } else {
            m_fileMode = MediumFile;
        }
    } else {
        m_fileMode = SmallFile;
    }

    // ================================================================
    // MODE 3: Large files (>100MB) — read-only, word wrap, no highlighting
    // Load the FULL file. QPlainTextEdit handles it IF word wrap is ON.
    // The hang was from QPlainTextEdit calculating the width of a
    // 93-million-character single line with word wrap OFF.
    // With word wrap ON, it only lays out the visible wrapped portion.
    // ================================================================
    if (m_fileMode == LargeFile) {
        // Don't load into PieceTable at all — Editor will load directly
        // into QTextDocument to avoid the double/triple copy that causes OOM
        m_filePath = filePath;
        m_modified = m_formatDirty = false;
        setReadOnly(true);

        // Detect encoding from first 8KB only
        QFile probe(filePath);
        if (!probe.open(QIODevice::ReadOnly)) return false;
        m_encoding = CharsetDetector::detect(probe.read(8192));
        probe.close();

        emit filePathChanged(m_filePath);
        emit encodingChanged(m_encoding);
        return true;
    }

    // ================================================================
    // MODE 2: Medium files (20-100MB) — direct load, no PieceTable dup
    // ================================================================
    if (m_fileMode == MediumFile) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) return false;

        QByteArray data = file.readAll();
        file.close();

        m_encoding = CharsetDetector::detect(data);
        QString text = QString::fromUtf8(data);
        data.clear(); // free byte array

        m_lineEnding = detectLineEnding(text, &m_mixedLineEndings);
        text = normalizeLineEndings(text, Unix);

        // Check for long lines — break them
        const int MAX_LINE = 8000;
        bool hasLongLine = false;
        int lineLen = 0;
        for (qsizetype i = 0; i < qMin(text.length(), qsizetype(100000)); ++i) {
            if (text.at(i) == '\n') { lineLen = 0; }
            else { lineLen++; if (lineLen > MAX_LINE) { hasLongLine = true; break; } }
        }
        if (hasLongLine) {
            setReadOnly(true);
        }

        m_content.setText(text);
        m_filePath = filePath;
        m_modified = m_formatDirty = false;
        emit filePathChanged(m_filePath);
        emit encodingChanged(m_encoding);
        return true;
    }

    // ================================================================
    // MODE 1: Small files (<20MB) — full features
    // ================================================================

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    // Detect encoding using charset detector
    m_encoding = CharsetDetector::detect(data);

    // Convert to QString
    QString text;
    if (m_encoding == "UTF-8" || m_encoding == "UTF-8-BOM") {
        QByteArray actualData = data;
        if (data.startsWith("\xEF\xBB\xBF")) {
            actualData = data.mid(3);
        }
        text = QString::fromUtf8(actualData);
    } else if (m_encoding == "UTF-16LE") {
        auto decoder = QStringDecoder(QStringDecoder::Utf16LE);
        text = decoder(data);
    } else if (m_encoding == "UTF-16BE") {
        auto decoder = QStringDecoder(QStringDecoder::Utf16BE);
        text = decoder(data);
    } else if (m_encoding == "ISO-8859-1") {
        text = QString::fromLatin1(data);
    } else {
        // Default to UTF-8
        text = QString::fromUtf8(data);
    }

    // Detect and normalize line endings
    m_lineEnding = detectLineEnding(text, &m_mixedLineEndings);
    text = normalizeLineEndings(text, Unix);  // Internally use Unix line endings

    m_content.setText(text);
    m_filePath = filePath;
    m_modified = m_formatDirty = false;

    emit filePathChanged(m_filePath);
    emit encodingChanged(m_encoding);
    emit lineEndingChanged(m_lineEnding);
    emit modifiedChanged(m_modified);
    emit contentsChanged();

    return true;
}

namespace {

// The 1.0.0 save-path encoder, moved out of saveAs unchanged so saveAs stays
// under 50 lines. T1.3 replaces it with TextCodec::encode.
QByteArray encodeForSave(const QString &text, const QString &encoding)
{
    if (encoding == "UTF-8-BOM") {
        return QByteArray("\xEF\xBB\xBF") + text.toUtf8();
    }
    if (encoding == "UTF-16LE") {
        auto encoder = QStringEncoder(QStringEncoder::Utf16LE);
        return encoder(text);
    }
    if (encoding == "UTF-16BE") {
        auto encoder = QStringEncoder(QStringEncoder::Utf16BE);
        return encoder(text);
    }
    if (encoding == "ISO-8859-1") {
        return text.toLatin1();
    }
    return text.toUtf8(); // UTF-8 and every unrecognised name, as in 1.0.0
}

// stat() follows symlinks, so these describe the file a save actually replaces.
bool targetExists(const QString &path, struct stat *st)
{
    return ::stat(QFile::encodeName(path).constData(), st) == 0;
}

bool targetWritable(const QString &path)
{
    return ::access(QFile::encodeName(path).constData(), W_OK) == 0;
}

} // namespace

SaveResult Document::save(WriteMode mode)
{
    if (m_filePath.isEmpty()) {
        m_lastError = tr("The document has no file name.");
        return SaveResult::IoError;
    }
    return saveAs(m_filePath, mode);
}

// A partial view holds only part of the file and a read-only document was
// opened from bytes it cannot reproduce; writing either would destroy data.
SaveResult Document::refusalFor() const
{
    if (isPartialView())
        return SaveResult::PartialView;
    if (m_readOnly)
        return SaveResult::ReadOnly;
    return SaveResult::Ok;
}

SaveResult Document::saveAs(const QString &filePath, WriteMode mode)
{
    m_directWriteAvailable = false;
    m_lastSaveWasDirect = false;
    const SaveResult refusal = refusalFor();
    if (refusal != SaveResult::Ok) {
        m_lastError = refusal == SaveResult::PartialView
            ? tr("Only part of this file is loaded, so it cannot be saved.")
            : tr("The document is read-only.");
        return refusal;
    }

    const QString text = normalizeLineEndings(m_content.text(), m_lineEnding);
    const QByteArray data = encodeForSave(text, m_encoding);

    struct stat st {};
    const bool exists = targetExists(filePath, &st);
    if (exists && !targetWritable(filePath)) {
        m_lastError = tr("File is read-only.");
        return SaveResult::IoError;
    }
    // An atomic replace would split a hard link or hand the file to us; the
    // user asked to save this file, so write it in place instead.
    const bool direct = exists && (st.st_nlink > 1 || st.st_uid != ::geteuid());
    const SaveResult result = direct ? writeDirect(filePath, data)
                                     : writeAtomic(filePath, data, mode);
    if (result != SaveResult::Ok)
        return result;

    m_lastSaveWasDirect = direct;
    finishSave(filePath);
    return SaveResult::Ok;
}

SaveResult Document::writeAtomic(const QString &path, const QByteArray &data, WriteMode mode)
{
    // QSaveFile::commit() fdatasyncs the temp file before the rename (checked
    // with strace on Qt 6.2.4), so no extra fsync is needed here.
    QSaveFile file(path);
    // Only after the user confirmed: QSaveFile then writes in place when it
    // cannot create its temp file next to the target.
    file.setDirectWriteFallback(mode == WriteMode::AllowDirect);
    if (!file.open(QIODevice::WriteOnly)) {
        m_lastError = file.errorString();
        // Offered only when the directory refused the temp file and the target
        // (already checked writable) exists. Never after ENOSPC or another
        // error, where a direct write would truncate the original.
        const QString dir = QFileInfo(path).absolutePath();
        m_directWriteAvailable = QFileInfo::exists(path) && !targetWritable(dir);
        return SaveResult::IoError;
    }
    if (file.write(data) != data.size() || !file.commit()) {
        m_lastError = file.errorString();
        file.cancelWriting();
        return SaveResult::IoError;
    }
    return SaveResult::Ok;
}

SaveResult Document::writeDirect(const QString &path, const QByteArray &data)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        m_lastError = file.errorString();
        return SaveResult::IoError;
    }
    const bool ok = file.write(data) == data.size() && file.flush()
        && ::fsync(file.handle()) == 0;
    if (!ok)
        m_lastError = file.errorString();
    file.close();
    return ok ? SaveResult::Ok : SaveResult::IoError;
}

void Document::finishSave(const QString &path)
{
    const QString oldPath = m_filePath;
    m_filePath = path;
    m_modified = m_formatDirty = false;
    if (oldPath != m_filePath)
        emit filePathChanged(m_filePath);
    emit modifiedChanged(m_modified);
    clearRecovery();
    m_lastError.clear();
}

bool Document::directWriteAvailable() const
{
    return m_directWriteAvailable;
}

bool Document::lastSaveWasDirect() const
{
    return m_lastSaveWasDirect;
}

bool Document::isFormatDirty() const
{
    return m_formatDirty;
}

bool Document::hadMixedLineEndings() const
{
    return m_mixedLineEndings;
}

// Placeholders (spec R2-1). These contracts are declared at the W0 base so later
// suites compile against it; each returns the 1.0.0 behaviour until the named
// task defines it.

bool Document::wasDecodedLossily() const
{
    return false; // T1.3
}

bool Document::reloadWithEncoding(const QString &name)
{
    Q_UNUSED(name);
    return false; // T1.3
}

void Document::setSizeThresholdsForTest(qint64 medium, qint64 large)
{
    Q_UNUSED(medium);
    Q_UNUSED(large); // T1.3
}

QString Document::fileName() const
{
    if (m_filePath.isEmpty()) {
        return QString();
    }
    return QFileInfo(m_filePath).fileName();
}

QString Document::displayName() const
{
    if (m_filePath.isEmpty()) {
        return tr("Untitled");
    }
    return fileName();
}

// A partial view cannot be saved, so a conversion would only mark it modified
// with no way to write it.
void Document::setEncoding(const QString &encoding)
{
    if (isPartialView())
        return;
    if (m_encoding != encoding) {
        m_encoding = encoding;
        setFormatDirty(true);
        emit encodingChanged(m_encoding);
    }
}

void Document::setLineEnding(LineEnding ending)
{
    if (isPartialView())
        return;
    if (m_lineEnding != ending) {
        m_lineEnding = ending;
        setFormatDirty(true);
        emit lineEndingChanged(m_lineEnding);
    }
}

QString Document::lineEndingString() const
{
    switch (m_lineEnding) {
    case Unix: return "\n";
    case Windows: return "\r\n";
    case ClassicMac: return "\r";
    }
    return "\n";
}

void Document::setText(const QString &text)
{
    m_content.setText(text);
    setModified(true);
    emit contentsChanged();
}

void Document::insert(qint64 position, const QString &text)
{
    m_content.insert(position, text);
    setModified(true);
    emit contentsChanged();
}

void Document::remove(qint64 position, qint64 length)
{
    m_content.remove(position, length);
    setModified(true);
    emit contentsChanged();
}

void Document::undo()
{
    m_content.undo();
    setModified(true);
    emit contentsChanged();
}

void Document::redo()
{
    m_content.redo();
    setModified(true);
    emit contentsChanged();
}

void Document::setLanguage(const QString &language)
{
    if (m_language != language) {
        m_language = language;
        emit languageChanged(m_language);
    }
}

void Document::setReadOnly(bool readOnly)
{
    if (m_readOnly != readOnly) {
        m_readOnly = readOnly;
        emit readOnlyChanged(m_readOnly);
    }
}

// Sets the content flag only. modifiedChanged reports isModified(), so a pending
// conversion keeps the document modified when the editor's undo reaches clean.
void Document::setModified(bool modified)
{
    const bool wasModified = isModified();
    m_modified = modified;
    if (isModified() != wasModified)
        emit modifiedChanged(isModified());
}

void Document::setFormatDirty(bool dirty)
{
    const bool wasModified = isModified();
    m_formatDirty = dirty;
    if (isModified() != wasModified)
        emit modifiedChanged(isModified());
}

bool Document::hasRecoveryFile() const
{
    return QFile::exists(recoveryFilePath());
}

bool Document::loadRecovery()
{
    QString recoveryPath = recoveryFilePath();
    if (!QFile::exists(recoveryPath)) {
        return false;
    }

    QFile file(recoveryPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QString text = QString::fromUtf8(file.readAll());
    file.close();

    m_content.setText(text);
    m_modified = true;
    emit modifiedChanged(m_modified);
    emit contentsChanged();

    return true;
}

void Document::saveRecovery()
{
    QString recoveryPath = recoveryFilePath();
    QDir().mkpath(QFileInfo(recoveryPath).absolutePath());

    QFile file(recoveryPath);
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    file.write(m_content.text().toUtf8());
    file.close();
}

void Document::clearRecovery()
{
    QString recoveryPath = recoveryFilePath();
    if (QFile::exists(recoveryPath)) {
        QFile::remove(recoveryPath);
    }
}

// The majority ending wins and *mixed records that more than one kind occurred,
// because saving normalises a mixed file to that majority. A file with no line
// break takes the configured default; 1.0.0 reported CRLF for every such file.
Document::LineEnding Document::detectLineEnding(const QString &text, bool *mixed)
{
    qint64 crlf = 0, lf = 0, cr = 0;

    for (qint64 i = 0; i < text.length(); ++i) {
        if (text[i] == '\r') {
            if (i + 1 < text.length() && text[i + 1] == '\n') {
                ++crlf;
                ++i;
            } else {
                ++cr;
            }
        } else if (text[i] == '\n') {
            ++lf;
        }
    }

    *mixed = (crlf > 0) + (lf > 0) + (cr > 0) > 1;
    if (crlf + lf + cr == 0) return defaultLineEnding();
    if (crlf >= lf && crlf >= cr) return Windows;
    if (cr > lf) return ClassicMac;
    return Unix;
}

Document::LineEnding Document::defaultLineEnding()
{
    const QString setting = Settings::instance().defaultLineEnding();
    if (setting == QLatin1String("CRLF")) return Windows;
    if (setting == QLatin1String("CR")) return ClassicMac;
    return Unix;
}

QString Document::normalizeLineEndings(const QString &text, LineEnding target)
{
    QString result;
    result.reserve(text.size());

    QString ending;
    switch (target) {
    case Unix: ending = "\n"; break;
    case Windows: ending = "\r\n"; break;
    case ClassicMac: ending = "\r"; break;
    }

    for (qint64 i = 0; i < text.length(); ++i) {
        if (text[i] == '\r') {
            result.append(ending);
            if (i + 1 < text.length() && text[i + 1] == '\n') {
                ++i;
            }
        } else if (text[i] == '\n') {
            result.append(ending);
        } else {
            result.append(text[i]);
        }
    }

    return result;
}

QString Document::recoveryFilePath() const
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString hash;

    if (m_filePath.isEmpty()) {
        hash = m_uuid;
    } else {
        hash = QString::fromLatin1(
            QCryptographicHash::hash(m_filePath.toUtf8(), QCryptographicHash::Md5).toHex());
    }

    return dataPath + "/recovery/" + hash + ".recovery";
}

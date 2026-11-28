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
#include <QSaveFile>
#include "textcodec.h"

#include <sys/stat.h>
#include <unistd.h>

namespace {

// The detector reports BOM presence separately; the encoding name carries it
// as a "-BOM" suffix so a save writes the BOM back.
QString detectedEncoding(const QByteArray &data)
{
    const CharsetGuess guess = CharsetDetector::detectCharset(data);
    return guess.hasBom ? guess.name + QStringLiteral("-BOM") : guess.name;
}

} // namespace

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
    if (fileSize > s_largeThreshold) {
        m_fileMode = LargeFile;
    } else if (fileSize > s_mediumThreshold) {
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
        m_encoding = detectedEncoding(probe.read(8192));
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

        QString text = decodeLoaded(data);
        data.clear(); // free byte array

        text = adoptLineEndings(text);

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

    // Detect the charset on the whole file and decode through TextCodec
    QString text = decodeLoaded(data);

    // Record line endings and normalise to LF (unless the T1.8 safety net trips)
    text = adoptLineEndings(text);

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
    const EncodeResult encoded = TextCodec::encode(text, m_encoding);
    if (!encoded.unmappable.isEmpty()) {
        m_lastError = tr("%n character(s) cannot be encoded in %1.", nullptr,
                         int(encoded.unmappable.size())).arg(m_encoding);
        return SaveResult::EncodeLossy;
    }
    const QByteArray &data = encoded.bytes;

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
    finishSave(filePath, data);
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

void Document::finishSave(const QString &path, const QByteArray &written)
{
    const QString oldPath = m_filePath;
    m_filePath = path;
    m_modified = m_formatDirty = false;
    if (oldPath != m_filePath)
        emit filePathChanged(m_filePath);
    emit modifiedChanged(m_modified);
    clearRecovery();
    m_lastError.clear();
    // DocumentManager records this to recognise the watcher event the save causes.
    emit saved(m_filePath, QCryptographicHash::hash(written, QCryptographicHash::Sha1),
               written.size());
}

bool Document::directWriteAvailable() const
{
    return m_directWriteAvailable;
}

bool Document::lastSaveWasDirect() const
{
    return m_lastSaveWasDirect;
}

Document::DiskState Document::lastSaveDiskState() const
{
    return DiskState::Unchanged;
}

QString Document::recoveryCopyPath() const
{
    return QString();
}

void Document::setReserveSpaceForTest(bool)
{
}

bool Document::isFormatDirty() const
{
    return m_formatDirty;
}

bool Document::hadMixedLineEndings() const
{
    return m_mixedLineEndings;
}

bool Document::wasDecodedLossily() const
{
    return m_decodedLossily;
}

// Decodes the loaded bytes with the detected charset, or the one forced by
// reloadWithEncoding. A decode that cannot reproduce the bytes opens the
// document read-only: saving it would rewrite the file with different bytes.
QString Document::decodeLoaded(const QByteArray &data)
{
    m_encoding = m_forcedEncoding.isEmpty() ? detectedEncoding(data) : m_forcedEncoding;
    DecodeResult decoded = TextCodec::decode(data, m_encoding);
    m_decodedLossily = !decoded.lossless;
    setReadOnly(m_decodedLossily);
    return decoded.text;
}

namespace {

bool isLineBreak(QChar c)
{
    return c == QLatin1Char('\r') || c == QLatin1Char('\n');
}

// CR NUL LF NUL is UTF-16 decoded as an 8-bit charset; real text does not put
// NUL beside a line break.
bool hasNulBesideLineBreak(const QString &text)
{
    for (qsizetype i = text.indexOf(QChar(0)); i >= 0; i = text.indexOf(QChar(0), i + 1)) {
        if ((i > 0 && isLineBreak(text.at(i - 1)))
            || (i + 1 < text.size() && isLineBreak(text.at(i + 1)))) {
            return true;
        }
    }
    return false;
}

} // namespace

// Records the file's line endings and normalises the text to LF (D9, D11).
// T1.8 safety net: text that looks like misdecoded UTF-16 is left untouched
// and opened read-only, because normalising its CR NUL LF NUL would double
// every line break on save. The UI offers Reopen with Encoding.
QString Document::adoptLineEndings(const QString &text)
{
    if (hasNulBesideLineBreak(text)) {
        m_lineEnding = defaultLineEnding();
        m_mixedLineEndings = false;
        m_decodedLossily = true;
        setReadOnly(true);
        return text;
    }
    m_lineEnding = detectLineEnding(text, &m_mixedLineEndings);
    return normalizeLineEndings(text, Unix);
}

// Re-reads the file's original bytes with the chosen codec. It is a reload,
// not a conversion: unsaved edits are discarded (the UI asks first).
bool Document::reloadWithEncoding(const QString &name)
{
    const QString canonical = TextCodec::canonicalName(name);
    if (canonical.isEmpty() || m_filePath.isEmpty() || isPartialView())
        return false;
    m_forcedEncoding = canonical;
    const bool loaded = load(m_filePath);
    m_forcedEncoding.clear();
    return loaded;
}

qint64 Document::s_mediumThreshold = Document::MEDIUM_FILE_THRESHOLD;
qint64 Document::s_largeThreshold = Document::LARGE_FILE_THRESHOLD;

// Tests force Medium mode on small fixtures. Pass MEDIUM_FILE_THRESHOLD and
// LARGE_FILE_THRESHOLD to restore the production values.
void Document::setSizeThresholdsForTest(qint64 medium, qint64 large)
{
    s_mediumThreshold = medium;
    s_largeThreshold = large;
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

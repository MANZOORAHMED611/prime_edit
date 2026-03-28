#include "document.h"
#include "charsetdetector.h"
#include "largefile.h"
#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QStringEncoder>
#include <QStringDecoder>

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
        m_fileMode = MediumFile;
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
        m_modified = false;
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

        m_lineEnding = detectLineEnding(text);
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
        m_modified = false;
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
    m_lineEnding = detectLineEnding(text);
    text = normalizeLineEndings(text, Unix);  // Internally use Unix line endings

    m_content.setText(text);
    m_filePath = filePath;
    m_modified = false;

    emit filePathChanged(m_filePath);
    emit encodingChanged(m_encoding);
    emit lineEndingChanged(m_lineEnding);
    emit modifiedChanged(m_modified);
    emit contentsChanged();

    return true;
}

bool Document::save()
{
    if (m_filePath.isEmpty()) {
        return false;
    }
    return saveAs(m_filePath);
}

bool Document::saveAs(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    // Get text and convert line endings
    QString text = m_content.text();
    text = normalizeLineEndings(text, m_lineEnding);

    // Convert encoding
    QByteArray data;
    if (m_encoding == "UTF-8") {
        data = text.toUtf8();
    } else if (m_encoding == "UTF-8-BOM") {
        data = QByteArray("\xEF\xBB\xBF") + text.toUtf8();
    } else if (m_encoding == "UTF-16LE") {
        auto encoder = QStringEncoder(QStringEncoder::Utf16LE);
        data = encoder(text);
    } else if (m_encoding == "UTF-16BE") {
        auto encoder = QStringEncoder(QStringEncoder::Utf16BE);
        data = encoder(text);
    } else if (m_encoding == "ISO-8859-1") {
        data = text.toLatin1();
    } else {
        data = text.toUtf8();
    }

    qint64 written = file.write(data);
    file.close();

    if (written != data.size()) {
        return false;
    }

    QString oldPath = m_filePath;
    m_filePath = filePath;
    m_modified = false;

    if (oldPath != m_filePath) {
        emit filePathChanged(m_filePath);
    }
    emit modifiedChanged(m_modified);
    clearRecovery();

    return true;
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

void Document::setEncoding(const QString &encoding)
{
    if (m_encoding != encoding) {
        m_encoding = encoding;
        setModified(true);
        emit encodingChanged(m_encoding);
    }
}

void Document::setLineEnding(LineEnding ending)
{
    if (m_lineEnding != ending) {
        m_lineEnding = ending;
        setModified(true);
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

void Document::setModified(bool modified)
{
    if (m_modified != modified) {
        m_modified = modified;
        emit modifiedChanged(m_modified);
    }
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

Document::LineEnding Document::detectLineEnding(const QString &text)
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

    if (crlf >= lf && crlf >= cr) return Windows;
    if (cr > lf) return ClassicMac;
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

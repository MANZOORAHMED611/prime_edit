#include "document.h"
#include "charsetdetector.h"
#include "largefile.h"
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

    // Large file mode — ANY file above threshold uses mmap or fails gracefully
    if (fi.size() > LARGE_FILE_THRESHOLD) {
        m_largeFileReader = new LargeFileReader(this);
        if (!m_largeFileReader->open(filePath)) {
            delete m_largeFileReader;
            m_largeFileReader = nullptr;
            // mmap failed — DO NOT fall through to readAll() for large files
            // readAll() would block the UI for minutes on a 100MB+ file
            // Instead, read just the first 200KB directly for preview
            QFile preview(filePath);
            if (preview.open(QIODevice::ReadOnly)) {
                QByteArray head = preview.read(200 * 1024);
                preview.close();
                m_filePath = filePath;
                m_encoding = CharsetDetector::detect(head);
                QString headText = QString::fromUtf8(head);
                int lastNL = headText.lastIndexOf('\n');
                if (lastNL > 0) headText.truncate(lastNL);
                m_content.setText(headText);
                m_modified = false;
                setReadOnly(true);
                emit filePathChanged(m_filePath);
                emit encodingChanged(m_encoding);
                return true;
            }
            return false;
        } else {
            m_filePath = filePath;
            m_encoding = m_largeFileReader->encoding();
            if (fi.size() > READONLY_FILE_THRESHOLD) {
                setReadOnly(true);
            }
            // Read first ~200KB directly from file for instant display
            // Don't use the line index (may not be ready yet)
            {
                QFile preview(filePath);
                if (preview.open(QIODevice::ReadOnly)) {
                    qint64 previewSize = qMin(fi.size(), qint64(200 * 1024));
                    QByteArray head = preview.read(previewSize);
                    preview.close();
                    QString headText = QString::fromUtf8(head);
                    int lastNL = headText.lastIndexOf('\n');
                    if (lastNL > 0) headText.truncate(lastNL);
                    m_content.setText(headText);
                }
            }
            m_modified = false;
            emit filePathChanged(m_filePath);
            emit encodingChanged(m_encoding);
            return true;
        }
    }

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

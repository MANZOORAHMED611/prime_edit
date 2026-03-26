#include "largefile.h"
#include <QFileInfo>

LargeFileReader::LargeFileReader(QObject *parent)
    : QObject(parent)
{
}

LargeFileReader::~LargeFileReader()
{
    close();
}

bool LargeFileReader::open(const QString &filePath)
{
    close();

    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::ReadOnly)) {
        return false;
    }

    m_fileSize = m_file.size();
    if (m_fileSize == 0) {
        m_file.close();
        return false;
    }

    m_mappedData = m_file.map(0, m_fileSize);
    if (!m_mappedData) {
        m_file.close();
        return false;
    }

    // Detect encoding from BOM
    if (m_fileSize >= 3 &&
        m_mappedData[0] == 0xEF &&
        m_mappedData[1] == 0xBB &&
        m_mappedData[2] == 0xBF) {
        m_encoding = "UTF-8-BOM";
    } else if (m_fileSize >= 2 &&
               m_mappedData[0] == 0xFF &&
               m_mappedData[1] == 0xFE) {
        m_encoding = "UTF-16LE";
    } else if (m_fileSize >= 2 &&
               m_mappedData[0] == 0xFE &&
               m_mappedData[1] == 0xFF) {
        m_encoding = "UTF-16BE";
    } else {
        m_encoding = "UTF-8";
    }

    buildLineIndex();
    return true;
}

void LargeFileReader::close()
{
    if (m_mappedData) {
        m_file.unmap(m_mappedData);
        m_mappedData = nullptr;
    }
    if (m_file.isOpen()) {
        m_file.close();
    }
    m_lineOffsets.clear();
    m_fileSize = 0;
}

void LargeFileReader::buildLineIndex()
{
    m_lineOffsets.clear();
    m_lineOffsets.reserve(static_cast<int>(m_fileSize / 40));

    m_lineOffsets.append(0);

    for (qint64 i = 0; i < m_fileSize; ++i) {
        if (m_mappedData[i] == '\n') {
            if (i + 1 < m_fileSize) {
                m_lineOffsets.append(i + 1);
            }
        }
    }
}

QString LargeFileReader::decodeLine(qint64 offset, qint64 length) const
{
    if (!m_mappedData || offset < 0 || offset >= m_fileSize) {
        return QString();
    }
    if (length <= 0) {
        return QString();
    }

    // Strip trailing \r\n or \n
    while (length > 0 &&
           (m_mappedData[offset + length - 1] == '\n' ||
            m_mappedData[offset + length - 1] == '\r')) {
        --length;
    }

    return QString::fromUtf8(
        reinterpret_cast<const char *>(m_mappedData + offset),
        static_cast<int>(length));
}

QString LargeFileReader::line(qint64 lineNumber) const
{
    if (lineNumber < 0 || lineNumber >= m_lineOffsets.size()) {
        return QString();
    }

    qint64 start = m_lineOffsets[lineNumber];
    qint64 end = (lineNumber + 1 < m_lineOffsets.size())
                     ? m_lineOffsets[lineNumber + 1]
                     : m_fileSize;

    return decodeLine(start, end - start);
}

QStringList LargeFileReader::lines(qint64 startLine, qint64 count) const
{
    QStringList result;
    result.reserve(static_cast<int>(count));

    for (qint64 i = startLine;
         i < startLine + count && i < m_lineOffsets.size();
         ++i) {
        result.append(line(i));
    }

    return result;
}

qint64 LargeFileReader::lineStartOffset(qint64 lineNumber) const
{
    if (lineNumber < 0 || lineNumber >= m_lineOffsets.size()) {
        return -1;
    }
    return m_lineOffsets[lineNumber];
}

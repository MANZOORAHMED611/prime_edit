#include "largefile.h"
#include <QFileInfo>
#ifdef Q_OS_LINUX
#include <sys/mman.h>
#endif

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
    m_sparseIndex = (m_fileSize > SPARSE_INDEX_THRESHOLD);

    if (m_sparseIndex) {
        // Sparse index: record every SPARSE_INTERVAL-th line offset
        // For a 1GB file with ~20M lines, this gives ~20K entries (~160KB)
        m_lineOffsets.reserve(static_cast<int>(m_fileSize / 40 / SPARSE_INTERVAL) + 1);
        m_lineOffsets.append(0);

        qint64 lineCount = 0;
        // Read in 1MB chunks to minimize page faulting
        constexpr qint64 CHUNK = 1024 * 1024;
        for (qint64 offset = 0; offset < m_fileSize; offset += CHUNK) {
            qint64 end = qMin(offset + CHUNK, m_fileSize);
            for (qint64 i = offset; i < end; ++i) {
                if (m_mappedData[i] == '\n') {
                    lineCount++;
                    if (lineCount % SPARSE_INTERVAL == 0 && i + 1 < m_fileSize) {
                        m_lineOffsets.append(i + 1);
                    }
                }
            }
            // Use madvise to release pages we've already scanned
#ifdef Q_OS_LINUX
            if (offset > CHUNK) {
                madvise(m_mappedData + offset - CHUNK, CHUNK, MADV_DONTNEED);
            }
#endif
        }
        m_totalLines = lineCount + 1;
    } else {
        // Full index for files under threshold
        qint64 estimatedLines = m_fileSize / 80; // conservative estimate
        m_lineOffsets.reserve(static_cast<int>(qMin(estimatedLines, qint64(50000000))));
        m_lineOffsets.append(0);

        for (qint64 i = 0; i < m_fileSize; ++i) {
            if (m_mappedData[i] == '\n') {
                if (i + 1 < m_fileSize) {
                    m_lineOffsets.append(i + 1);
                }
            }
        }
        m_totalLines = m_lineOffsets.size();
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

qint64 LargeFileReader::resolveLineOffset(qint64 lineNumber) const
{
    if (!m_sparseIndex) {
        // Full index — direct lookup
        if (lineNumber < 0 || lineNumber >= m_lineOffsets.size()) return -1;
        return m_lineOffsets[lineNumber];
    }

    // Sparse index — find nearest checkpoint, then scan forward
    qint64 checkpointIndex = lineNumber / SPARSE_INTERVAL;
    if (checkpointIndex >= m_lineOffsets.size()) checkpointIndex = m_lineOffsets.size() - 1;
    qint64 offset = m_lineOffsets[checkpointIndex];
    qint64 currentLine = checkpointIndex * SPARSE_INTERVAL;

    // Scan forward to the requested line
    while (currentLine < lineNumber && offset < m_fileSize) {
        if (m_mappedData[offset] == '\n') {
            currentLine++;
        }
        offset++;
    }

    return (currentLine == lineNumber && offset <= m_fileSize) ? offset : -1;
}

QString LargeFileReader::line(qint64 lineNumber) const
{
    if (lineNumber < 0 || lineNumber >= m_totalLines) return QString();

    qint64 start = resolveLineOffset(lineNumber);
    if (start < 0) return QString();

    // Find end of line
    qint64 end = start;
    while (end < m_fileSize && m_mappedData[end] != '\n') {
        end++;
    }

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
    return resolveLineOffset(lineNumber);
}

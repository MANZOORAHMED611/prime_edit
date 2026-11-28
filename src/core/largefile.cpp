#include "largefile.h"
#include <QFileInfo>
#include <QThread>
#include <QtConcurrent>
#include <QCoreApplication>
#include <QEventLoop>
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

    // Quick initial estimate of line count (assume ~80 bytes per line)
    m_totalLines = m_fileSize / 80;
    m_sparseIndex = true;
    m_lineOffsets.clear();
    m_lineOffsets.append(0); // First line at offset 0
    m_indexReady = false;

    // Build the full index in a background thread — UI stays responsive
    buildLineIndexAsync();

    return true;
}

void LargeFileReader::close()
{
    // The index worker reads m_mappedData: stop it before unmapping.
    m_cancelIndex = true;
    m_indexFuture.waitForFinished();
    m_indexFuture = QFuture<void>();
    if (m_mappedData) {
        m_file.unmap(m_mappedData);
        m_mappedData = nullptr;
    }
    if (m_file.isOpen()) {
        m_file.close();
    }
    m_lineOffsets.clear();
    m_fileSize = 0;
    m_totalLines = 0;
    m_indexReady = false;
}

void LargeFileReader::buildLineIndexAsync()
{
    m_cancelIndex = false;
    const uchar *data = m_mappedData;
    const qint64 size = m_fileSize;
    m_indexFuture = QtConcurrent::run([this, data, size]() {
        LineIndex index = computeLineIndex(data, size, m_cancelIndex);
        if (m_cancelIndex) {
            return;
        }
        // Publish on the owning thread. If this object is destroyed first,
        // Qt drops the queued call (close() has already waited for us).
        QMetaObject::invokeMethod(this, [this, index]() { publishIndex(index); },
                                  Qt::QueuedConnection);
    });
}

void LargeFileReader::publishIndex(const LineIndex &index)
{
    if (!m_mappedData) {
        return; // closed or reopened after the build was queued
    }
    m_lineOffsets = index.offsets;
    m_totalLines = index.totalLines;
    m_indexReady = true;
    emit indexBuildComplete();
}

bool LargeFileReader::waitForIndex()
{
    if (!m_indexFuture.isValid() && !m_indexReady) {
        return false;
    }
    m_indexFuture.waitForFinished();
    // The result arrives as a queued call on this thread; deliver it.
    while (!m_indexReady && m_mappedData && !m_cancelIndex) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
    return m_indexReady;
}

LargeFileReader::LineIndex LargeFileReader::computeLineIndex(
    const uchar *data, qint64 size, const std::atomic<bool> &cancel)
{
    LineIndex index;
    index.offsets.reserve(static_cast<int>(size / 40 / SPARSE_INTERVAL) + 1);
    index.offsets.append(0);
    qint64 lineCount = 0;
    constexpr qint64 CHUNK = 1024 * 1024;
    for (qint64 offset = 0; offset < size; offset += CHUNK) {
        if (cancel) {
            return index;
        }
        const qint64 end = qMin(offset + CHUNK, size);
        for (qint64 i = offset; i < end; ++i) {
            if (data[i] == '\n') {
                lineCount++;
                if (lineCount % SPARSE_INTERVAL == 0 && i + 1 < size) {
                    index.offsets.append(i + 1);
                }
            }
        }
#ifdef Q_OS_LINUX
        // Release pages already scanned; the mapping stays valid.
        if (offset > CHUNK) {
            madvise(const_cast<uchar *>(data) + offset - CHUNK, CHUNK, MADV_DONTNEED);
        }
#endif
    }
    index.totalLines = lineCount + 1;
    return index;
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
         i < startLine + count && i < m_totalLines;
         ++i) {
        result.append(line(i));
    }

    return result;
}

qint64 LargeFileReader::lineStartOffset(qint64 lineNumber) const
{
    return resolveLineOffset(lineNumber);
}

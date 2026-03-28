#ifndef LARGEFILE_H
#define LARGEFILE_H

#include <QObject>
#include <QFile>
#include <QVector>
#include <QString>
#include <QStringList>

class LargeFileReader : public QObject
{
    Q_OBJECT
public:
    explicit LargeFileReader(QObject *parent = nullptr);
    ~LargeFileReader() override;

    bool open(const QString &filePath);
    void close();
    bool isOpen() const { return m_mappedData != nullptr; }

    qint64 fileSize() const { return m_fileSize; }
    qint64 lineCount() const { return m_totalLines; }
    QString encoding() const { return m_encoding; }

    QString line(qint64 lineNumber) const;
    QStringList lines(qint64 startLine, qint64 count) const;
    qint64 lineStartOffset(qint64 lineNumber) const;

    // Constants — ALL large files use sparse index for fast open
    static constexpr qint64 SPARSE_INDEX_THRESHOLD = 0; // Always sparse for large files
    static constexpr int SPARSE_INTERVAL = 1000; // index every 1000th line

    bool isIndexReady() const { return m_indexReady; }

signals:
    void indexBuildComplete();

private:
    void buildLineIndex();
    void buildLineIndexAsync();
    qint64 resolveLineOffset(qint64 lineNumber) const;
    QString decodeLine(qint64 offset, qint64 length) const;

    QFile m_file;
    uchar *m_mappedData = nullptr;
    qint64 m_fileSize = 0;
    QVector<qint64> m_lineOffsets;
    qint64 m_totalLines = 0;
    bool m_sparseIndex = false;
    bool m_indexReady = false;
    QString m_encoding = "UTF-8";
};

#endif // LARGEFILE_H

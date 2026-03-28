#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QObject>
#include <QString>
#include <QFileInfo>
#include <QUuid>
#include <memory>
#include "piecetable.h"

class LargeFileReader;

class Document : public QObject
{
    Q_OBJECT

public:
    enum LineEnding {
        Unix,       // LF (\n)
        Windows,    // CRLF (\r\n)
        ClassicMac  // CR (\r)
    };

    // Three file modes:
    // 1. Small (<20MB): full features — PieceTable, syntax highlighting, undo
    // 2. Medium (20-100MB): reduced — no PieceTable copy, no highlighting, limited undo
    // 3. Large/Extreme (>100MB OR single mega-line): viewer mode — chunked, read-only
    static constexpr qint64 MEDIUM_FILE_THRESHOLD = 20 * 1024 * 1024;     // 20MB
    static constexpr qint64 LARGE_FILE_THRESHOLD = 50 * 1024 * 1024;      // 50MB
    static constexpr qint64 READONLY_FILE_THRESHOLD = 100 * 1024 * 1024;  // same as large

    enum FileMode { SmallFile, MediumFile, LargeFile };
    FileMode fileMode() const { return m_fileMode; }

    explicit Document(QObject *parent = nullptr);
    ~Document() override;

    // File operations
    bool load(const QString &filePath);
    bool save();
    bool saveAs(const QString &filePath);
    bool isModified() const { return m_modified; }
    bool isUntitled() const { return m_filePath.isEmpty(); }

    // File info
    QString filePath() const { return m_filePath; }
    QString fileName() const;
    QString displayName() const;

    // Encoding
    QString encoding() const { return m_encoding; }
    void setEncoding(const QString &encoding);
    LineEnding lineEnding() const { return m_lineEnding; }
    void setLineEnding(LineEnding ending);
    QString lineEndingString() const;

    // Content access
    PieceTable *content() { return &m_content; }
    const PieceTable *content() const { return &m_content; }
    QString text() const { return m_content.text(); }
    void setText(const QString &text);

    // Editing
    void insert(qint64 position, const QString &text);
    void remove(qint64 position, qint64 length);

    // Undo/Redo
    bool canUndo() const { return m_content.canUndo(); }
    bool canRedo() const { return m_content.canRedo(); }
    void undo();
    void redo();

    // Language
    QString language() const { return m_language; }
    void setLanguage(const QString &language);

    // Read-only mode
    bool isReadOnly() const { return m_readOnly; }
    void setReadOnly(bool readOnly);

    // Large file support
    bool isLargeFile() const { return m_largeFileReader != nullptr; }
    LargeFileReader *largeFileReader() const { return m_largeFileReader; }

    // Recovery
    bool hasRecoveryFile() const;
    bool loadRecovery();
    void saveRecovery();
    void clearRecovery();

signals:
    void modifiedChanged(bool modified);
    void filePathChanged(const QString &path);
    void encodingChanged(const QString &encoding);
    void lineEndingChanged(LineEnding ending);
    void languageChanged(const QString &language);
    void readOnlyChanged(bool readOnly);
    void contentsChanged();

public slots:
    void setModified(bool modified);

private:
    PieceTable m_content;
    QString m_filePath;
    QString m_encoding = "UTF-8";
    LineEnding m_lineEnding = Unix;
    QString m_language;
    bool m_modified = false;
    bool m_readOnly = false;
    FileMode m_fileMode = SmallFile;
    LargeFileReader *m_largeFileReader = nullptr;
    QString m_uuid;

    static LineEnding detectLineEnding(const QString &text);
    QString normalizeLineEndings(const QString &text, LineEnding target);
    QString recoveryFilePath() const;
};

#endif // DOCUMENT_H

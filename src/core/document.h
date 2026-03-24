#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QObject>
#include <QString>
#include <QFileInfo>
#include <memory>
#include "piecetable.h"

class Document : public QObject
{
    Q_OBJECT

public:
    enum LineEnding {
        Unix,       // LF (\n)
        Windows,    // CRLF (\r\n)
        ClassicMac  // CR (\r)
    };

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

    static QString detectEncoding(const QByteArray &data);
    static LineEnding detectLineEnding(const QString &text);
    QString normalizeLineEndings(const QString &text, LineEnding target);
    QString recoveryFilePath() const;
};

#endif // DOCUMENT_H

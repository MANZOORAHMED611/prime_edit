#ifndef SEARCHENGINE_H
#define SEARCHENGINE_H

#include <QObject>
#include <QString>
#include <QRegularExpression>
#include <QVector>

struct SearchResult {
    QString filePath;   // empty for current document
    int line = 0;       // 1-based
    int column = 0;     // 0-based
    int length = 0;     // match length
    QString lineText;   // full line content
};

class SearchEngine : public QObject
{
    Q_OBJECT
public:
    enum Mode { Normal, Extended, Regex };

    struct Options {
        Mode mode = Normal;
        bool caseSensitive = false;
        bool wholeWord = false;
        bool wrapAround = true;
        bool dotMatchesNewline = false;
        bool searchBackward = false;
    };

    explicit SearchEngine(QObject *parent = nullptr);

    // Single document search
    QVector<SearchResult> findAll(const QString &text, const QString &pattern, const Options &opts);
    SearchResult findNext(const QString &text, int fromPos, const QString &pattern, const Options &opts);
    int matchCount(const QString &text, const QString &pattern, const Options &opts);

    // Replace
    QString replaceInText(const QString &text, const QString &pattern,
                         const QString &replacement, const Options &opts);

    // Find in files
    void findInFiles(const QString &directory, const QString &pattern,
                     const QString &fileFilter, const Options &opts,
                     bool recursive, bool includeHidden);

    // Large file search (runs on memory-mapped data)
    void searchLargeFile(const QString &filePath, const QString &pattern,
                         const Options &opts);

    // Utility
    static QString expandEscapes(const QString &text);

signals:
    void fileSearchResult(const SearchResult &result);
    void fileSearchFinished(int totalHits, int totalFiles);
    void fileSearchProgress(const QString &currentFile);

    void largeFileSearchResult(const SearchResult &result);
    void largeFileSearchFinished(int totalHits);
    void largeFileSearchProgress(int percent);

private:
    QRegularExpression buildRegex(const QString &pattern, const Options &opts) const;
};

#endif

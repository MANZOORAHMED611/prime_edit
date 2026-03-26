#include "searchengine.h"
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>

SearchEngine::SearchEngine(QObject *parent)
    : QObject(parent)
{
}

QString SearchEngine::expandEscapes(const QString &text)
{
    QString result;
    result.reserve(text.length());

    for (int i = 0; i < text.length(); ++i) {
        if (text.at(i) == '\\' && i + 1 < text.length()) {
            QChar next = text.at(i + 1);
            if (next == 'n') { result += '\n'; ++i; }
            else if (next == 'r') { result += '\r'; ++i; }
            else if (next == 't') { result += '\t'; ++i; }
            else if (next == '0') { result += QChar(0); ++i; }
            else if (next == '\\') { result += '\\'; ++i; }
            else if (next == 'x' && i + 3 < text.length()) {
                QString hex = text.mid(i + 2, 2);
                bool ok;
                int val = hex.toInt(&ok, 16);
                if (ok) {
                    result += QChar(val);
                    i += 3;
                } else {
                    result += text.at(i);
                }
            } else {
                result += text.at(i);
            }
        } else {
            result += text.at(i);
        }
    }
    return result;
}

QRegularExpression SearchEngine::buildRegex(
    const QString &pattern, const Options &opts) const
{
    QString regexPattern;

    switch (opts.mode) {
    case Normal:
        regexPattern = QRegularExpression::escape(pattern);
        break;
    case Extended:
        regexPattern = QRegularExpression::escape(expandEscapes(pattern));
        break;
    case Regex:
        regexPattern = pattern;
        break;
    }

    if (opts.wholeWord) {
        regexPattern = "\\b" + regexPattern + "\\b";
    }

    QRegularExpression::PatternOptions regexOpts =
        QRegularExpression::NoPatternOption;
    if (!opts.caseSensitive) {
        regexOpts |= QRegularExpression::CaseInsensitiveOption;
    }
    if (opts.dotMatchesNewline) {
        regexOpts |= QRegularExpression::DotMatchesEverythingOption;
    }

    return QRegularExpression(regexPattern, regexOpts);
}

QVector<SearchResult> SearchEngine::findAll(
    const QString &text, const QString &pattern, const Options &opts)
{
    QVector<SearchResult> results;
    if (pattern.isEmpty()) return results;

    QRegularExpression regex = buildRegex(pattern, opts);
    if (!regex.isValid()) return results;

    QRegularExpressionMatchIterator it = regex.globalMatch(text);
    QStringList lines = text.split('\n');

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();

        int pos = match.capturedStart();
        int lineNum = 1;
        int lineStart = 0;
        for (int i = 0; i < pos; ++i) {
            if (text.at(i) == '\n') {
                lineNum++;
                lineStart = i + 1;
            }
        }

        SearchResult result;
        result.line = lineNum;
        result.column = pos - lineStart;
        result.length = match.capturedLength();
        result.lineText = (lineNum - 1 < lines.size())
                              ? lines.at(lineNum - 1)
                              : QString();
        results.append(result);
    }

    return results;
}

SearchResult SearchEngine::findNext(
    const QString &text, int fromPos,
    const QString &pattern, const Options &opts)
{
    SearchResult result;
    if (pattern.isEmpty()) return result;

    QRegularExpression regex = buildRegex(pattern, opts);
    if (!regex.isValid()) return result;

    QRegularExpressionMatch match;

    if (opts.searchBackward) {
        QRegularExpressionMatchIterator it = regex.globalMatch(text);
        int bestPos = -1;
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            if (m.capturedStart() < fromPos) {
                bestPos = m.capturedStart();
                match = m;
            }
        }
        if (bestPos < 0 && opts.wrapAround) {
            it = regex.globalMatch(text);
            while (it.hasNext()) {
                match = it.next();
            }
        }
        if (!match.hasMatch()) return result;
    } else {
        match = regex.match(text, fromPos);
        if (!match.hasMatch() && opts.wrapAround) {
            match = regex.match(text, 0);
        }
        if (!match.hasMatch()) return result;
    }

    int pos = match.capturedStart();
    int lineNum = 1;
    int lineStart = 0;
    for (int i = 0; i < pos && i < text.length(); ++i) {
        if (text.at(i) == '\n') {
            lineNum++;
            lineStart = i + 1;
        }
    }

    QStringList lines = text.split('\n');
    result.line = lineNum;
    result.column = pos - lineStart;
    result.length = match.capturedLength();
    result.lineText = (lineNum - 1 < lines.size())
                          ? lines.at(lineNum - 1)
                          : QString();

    return result;
}

int SearchEngine::matchCount(
    const QString &text, const QString &pattern, const Options &opts)
{
    return findAll(text, pattern, opts).size();
}

QString SearchEngine::replaceInText(
    const QString &text, const QString &pattern,
    const QString &replacement, const Options &opts)
{
    QRegularExpression regex = buildRegex(pattern, opts);
    if (!regex.isValid()) return text;

    QString rep = replacement;
    if (opts.mode == Extended) {
        rep = expandEscapes(rep);
    }

    return QString(text).replace(regex, rep);
}

void SearchEngine::findInFiles(
    const QString &directory, const QString &pattern,
    const QString &fileFilter, const Options &opts,
    bool recursive, bool includeHidden)
{
    if (pattern.isEmpty()) {
        emit fileSearchFinished(0, 0);
        return;
    }

    QRegularExpression regex = buildRegex(pattern, opts);
    if (!regex.isValid()) {
        emit fileSearchFinished(0, 0);
        return;
    }

    QStringList filters = fileFilter.split(';', Qt::SkipEmptyParts);
    for (QString &f : filters) {
        f = f.trimmed();
    }
    if (filters.isEmpty()) {
        filters << "*";
    }

    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if (recursive) flags |= QDirIterator::Subdirectories;

    QDir::Filters dirFilters = QDir::Files | QDir::Readable;
    if (includeHidden) dirFilters |= QDir::Hidden;

    int totalHits = 0;
    int totalFiles = 0;

    QDirIterator dirIt(directory, filters, dirFilters, flags);

    while (dirIt.hasNext()) {
        QString filePath = dirIt.next();
        emit fileSearchProgress(filePath);

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

        QTextStream stream(&file);
        QString content = stream.readAll();
        file.close();

        QVector<SearchResult> results = findAll(content, pattern, opts);
        if (!results.isEmpty()) {
            totalFiles++;
            for (SearchResult &r : results) {
                r.filePath = filePath;
                emit fileSearchResult(r);
                totalHits++;
            }
        }
    }

    emit fileSearchFinished(totalHits, totalFiles);
}

#include "texttransforms.h"
#include "texttransforms_p.h"

namespace TextTransforms {

using detail::eachLine;
using detail::isBlank;
using detail::mapLines;

namespace detail {

QString mapLines(const QString &text, const std::function<QStringList(const QStringList &)> &fn)
{
    SplitText split = splitLines(text);
    split.lines = fn(split.lines);
    if (split.lines.isEmpty()) {
        // Every line was removed: what is left is at most the trailing EOL.
        return split.trailingEol ? split.eol : QString();
    }
    return joinLines(split);
}

QString eachLine(const QString &text, const std::function<QString(const QString &)> &fn)
{
    return mapLines(text, [&fn](const QStringList &lines) {
        QStringList out;
        out.reserve(lines.size());
        for (const QString &line : lines)
            out.append(fn(line));
        return out;
    });
}

} // namespace detail

namespace {

int leadingBlanks(const QString &line)
{
    int i = 0;
    while (i < line.size() && isBlank(line.at(i)))
        ++i;
    return i;
}

int trailingBlanksStart(const QString &line)
{
    int end = line.size();
    while (end > 0 && isBlank(line.at(end - 1)))
        --end;
    return end;
}

QStringList filterLines(const QStringList &lines, const std::function<bool(const QString &)> &keep)
{
    QStringList out;
    for (const QString &line : lines) {
        if (keep(line))
            out.append(line);
    }
    return out;
}

int nextTabStop(int column, int tabWidth)
{
    return (column / tabWidth + 1) * tabWidth;
}

QString expandTabs(const QString &line, int tabWidth)
{
    QString out;
    for (QChar ch : line) {
        if (ch == QLatin1Char('\t'))
            out.append(QString(nextTabStop(int(out.size()), tabWidth) - int(out.size()), QLatin1Char(' ')));
        else
            out.append(ch);
    }
    return out;
}

// Spaces are held back until the run ends. A run that reaches a tab stop
// with two or more spaces becomes a tab; one followed by a tab is dropped,
// since the tab reaches the same stop.
QString compressSpaces(const QString &line, int tabWidth, bool leadingOnly)
{
    QString out;
    int column = 0;
    int pending = 0;
    const int stop = leadingOnly ? leadingBlanks(line) : int(line.size());
    for (int i = 0; i < line.size(); ++i) {
        const QChar ch = line.at(i);
        if (i >= stop) {
            out.append(QString(pending, QLatin1Char(' '))).append(line.mid(i));
            return out;
        }
        if (ch == QLatin1Char(' ')) {
            ++pending;
            ++column;
            if (column % tabWidth == 0 && pending >= 2) {
                out.append(QLatin1Char('\t'));
                pending = 0;
            } else if (column % tabWidth == 0) {
                out.append(QString(pending, QLatin1Char(' ')));
                pending = 0;
            }
            continue;
        }
        if (ch == QLatin1Char('\t')) {
            pending = 0;
            column = nextTabStop(column, tabWidth);
        } else {
            out.append(QString(pending, QLatin1Char(' ')));
            pending = 0;
            ++column;
        }
        out.append(ch);
    }
    return out.append(QString(pending, QLatin1Char(' ')));
}

} // namespace

SplitText splitLines(const QString &text)
{
    SplitText split;
    bool eolSeen = false;
    QString current;
    for (int i = 0; i < text.size(); ++i) {
        const QChar ch = text.at(i);
        if (ch != QLatin1Char('\n') && ch != QLatin1Char('\r')) {
            current.append(ch);
            continue;
        }
        const bool crlf = ch == QLatin1Char('\r') && i + 1 < text.size()
                          && text.at(i + 1) == QLatin1Char('\n');
        if (!eolSeen) {
            split.eol = crlf ? QStringLiteral("\r\n") : QString(ch);
            eolSeen = true;
        }
        if (crlf)
            ++i;
        split.lines.append(current);
        current.clear();
    }
    split.trailingEol = eolSeen && current.isEmpty();
    if (!split.trailingEol || split.lines.isEmpty())
        split.lines.append(current);
    return split;
}

QString joinLines(const SplitText &split)
{
    QString text = split.lines.join(split.eol);
    if (split.trailingEol)
        text.append(split.eol);
    return text;
}

QString removeEmptyLines(const QString &text)
{
    return mapLines(text, [](const QStringList &lines) {
        return filterLines(lines, [](const QString &line) { return !line.isEmpty(); });
    });
}

QString removeBlankLines(const QString &text)
{
    return mapLines(text, [](const QStringList &lines) {
        return filterLines(lines, [](const QString &line) {
            return leadingBlanks(line) < line.size();
        });
    });
}

QString removeConsecutiveDuplicateLines(const QString &text)
{
    return mapLines(text, [](const QStringList &lines) {
        QStringList out;
        for (const QString &line : lines) {
            if (out.isEmpty() || out.constLast() != line)
                out.append(line);
        }
        return out;
    });
}

QString trimTrailing(const QString &text)
{
    return eachLine(text, [](const QString &line) { return line.left(trailingBlanksStart(line)); });
}

QString trimLeading(const QString &text)
{
    return eachLine(text, [](const QString &line) { return line.mid(leadingBlanks(line)); });
}

QString trimBoth(const QString &text)
{
    return trimLeading(trimTrailing(text));
}

QString eolToSpace(const QString &text)
{
    const SplitText split = splitLines(text);
    QString joined = split.lines.join(QLatin1Char(' '));
    if (split.trailingEol)
        joined.append(split.eol);
    return joined;
}

QString trimBothAndEolToSpace(const QString &text)
{
    return eolToSpace(trimBoth(text));
}

QString formatDateTime(const QDateTime &when, const QLocale &locale, bool longFormat)
{
    return locale.toString(when.time(), QLocale::ShortFormat) + QLatin1Char(' ')
           + locale.toString(when.date(), longFormat ? QLocale::LongFormat : QLocale::ShortFormat);
}

QString tabsToSpaces(const QString &text, int tabWidth)
{
    const int width = qMax(1, tabWidth);
    return eachLine(text, [width](const QString &line) { return expandTabs(line, width); });
}

QString spacesToTabs(const QString &text, int tabWidth, bool leadingOnly)
{
    const int width = qMax(1, tabWidth);
    return eachLine(text, [width, leadingOnly](const QString &line) {
        return compressSpaces(line, width, leadingOnly);
    });
}

} // namespace TextTransforms

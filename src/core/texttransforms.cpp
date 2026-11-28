#include "texttransforms.h"

#include <functional>

namespace TextTransforms {

namespace {

// Notepad++ blank operations treat only space and tab as blanks. U+00A0 is
// text the user typed on purpose (spec D11), so it is never trimmed.
bool isBlank(QChar ch)
{
    return ch == QLatin1Char(' ') || ch == QLatin1Char('\t');
}

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

// Applies fn to the line list and rejoins with the original EOL convention.
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

QStringList filterLines(const QStringList &lines, const std::function<bool(const QString &)> &keep)
{
    QStringList out;
    for (const QString &line : lines) {
        if (keep(line))
            out.append(line);
    }
    return out;
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

} // namespace TextTransforms

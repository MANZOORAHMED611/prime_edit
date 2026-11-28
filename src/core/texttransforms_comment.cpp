// Comment and indentation transforms of TextTransforms (texttransforms.h),
// following the Notepad++ Comment/Uncomment and Indent commands.
#include "texttransforms.h"
#include "texttransforms_p.h"

namespace TextTransforms {

namespace {

int indentOf(const QString &line)
{
    int i = 0;
    while (i < line.size() && detail::isBlank(line.at(i)))
        ++i;
    return i;
}

bool isBlankLine(const QString &line)
{
    return indentOf(line) == line.size();
}

// Applies fn to every line, except that blank (or, for indent, empty) lines
// are left alone when there is more than one line, as in Notepad++.
QString eachNonBlankLine(const QString &text, const std::function<bool(const QString &)> &skip,
                         const std::function<QString(const QString &)> &fn)
{
    return detail::mapLines(text, [&skip, &fn](const QStringList &lines) {
        QStringList out;
        for (const QString &line : lines)
            out.append(lines.size() > 1 && skip(line) ? line : fn(line));
        return out;
    });
}

// Removes `token` from the start of `text` and one space after it.
QString dropPrefix(const QString &text, const QString &token)
{
    const int at = int(token.size());
    return text.mid(at < text.size() && text.at(at) == QLatin1Char(' ') ? at + 1 : at);
}

// Removes `token` from the end of `text` and one space before it.
QString dropSuffix(const QString &text, const QString &token)
{
    const int end = int(text.size() - token.size());
    return text.left(end > 0 && text.at(end - 1) == QLatin1Char(' ') ? end - 1 : end);
}

bool isOuterSpace(QChar ch)
{
    return detail::isBlank(ch) || ch == QLatin1Char('\n') || ch == QLatin1Char('\r');
}

// text split into leading whitespace, core and trailing whitespace.
struct Padded {
    QString lead;
    QString core;
    QString trail;
};

Padded splitPadding(const QString &text)
{
    int start = 0;
    while (start < text.size() && isOuterSpace(text.at(start)))
        ++start;
    int end = int(text.size());
    while (end > start && isOuterSpace(text.at(end - 1)))
        --end;
    return {text.left(start), text.mid(start, end - start), text.mid(end)};
}

std::optional<QString> unwrap(const QString &core, const CommentSyntax &syntax)
{
    if (core.size() < syntax.blockStart.size() + syntax.blockEnd.size()
        || !core.startsWith(syntax.blockStart) || !core.endsWith(syntax.blockEnd)) {
        return std::nullopt;
    }
    return dropSuffix(dropPrefix(core, syntax.blockStart), syntax.blockEnd);
}

} // namespace

QString indentLines(const QString &text, const QString &unit)
{
    return eachNonBlankLine(text, [](const QString &line) { return line.isEmpty(); },
                            [&unit](const QString &line) { return unit + line; });
}

QString unindentLines(const QString &text, int tabWidth)
{
    const int width = qMax(1, tabWidth);
    return detail::eachLine(text, [width](const QString &line) {
        if (line.startsWith(QLatin1Char('\t')))
            return line.mid(1);
        int spaces = 0;
        while (spaces < line.size() && line.at(spaces) == QLatin1Char(' '))
            ++spaces;
        const int remove = spaces % width ? spaces % width : qMin(spaces, width);
        return line.mid(remove);
    });
}

QString commentLines(const QString &text, const CommentSyntax &syntax)
{
    if (!syntax.hasLine() && !syntax.hasBlock())
        return text;
    return eachNonBlankLine(text, isBlankLine, [&syntax](const QString &line) {
        const int indent = indentOf(line);
        const QString body = line.mid(indent);
        if (syntax.hasLine())
            return line.left(indent) + syntax.line + QLatin1Char(' ') + body;
        return line.left(indent) + syntax.blockStart + QLatin1Char(' ') + body + QLatin1Char(' ')
               + syntax.blockEnd;
    });
}

QString uncommentLines(const QString &text, const CommentSyntax &syntax)
{
    return detail::eachLine(text, [&syntax](const QString &line) {
        const int indent = indentOf(line);
        const QString body = line.mid(indent);
        if (syntax.hasLine())
            return body.startsWith(syntax.line) ? line.left(indent) + dropPrefix(body, syntax.line) : line;
        if (!syntax.hasBlock())
            return line;
        const Padded padded = splitPadding(body);
        const std::optional<QString> inner = unwrap(padded.core, syntax);
        return inner ? line.left(indent) + *inner + padded.trail : line;
    });
}

QString blockComment(const QString &text, const CommentSyntax &syntax)
{
    const Padded padded = splitPadding(text);
    if (!syntax.hasBlock() || padded.core.isEmpty())
        return text;
    return padded.lead + syntax.blockStart + QLatin1Char(' ') + padded.core + QLatin1Char(' ')
           + syntax.blockEnd + padded.trail;
}

std::optional<QString> blockUncomment(const QString &text, const CommentSyntax &syntax)
{
    if (!syntax.hasBlock())
        return std::nullopt;
    const Padded padded = splitPadding(text);
    const std::optional<QString> inner = unwrap(padded.core, syntax);
    if (!inner)
        return std::nullopt;
    return padded.lead + *inner + padded.trail;
}

} // namespace TextTransforms

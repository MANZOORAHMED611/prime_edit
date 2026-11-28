// Bookmarked-line and brace functions of TextTransforms (texttransforms.h),
// behind the Notepad++ Search > Bookmark and brace commands.
#include "texttransforms.h"
#include "texttransforms_p.h"

#include <algorithm>

namespace TextTransforms {

QString keepLines(const QString &text, const QSet<int> &lines, bool keepListed)
{
    return detail::mapLines(text, [&lines, keepListed](const QStringList &all) {
        QStringList out;
        for (int i = 0; i < all.size(); ++i) {
            if (lines.contains(i) == keepListed)
                out.append(all.at(i));
        }
        return out;
    });
}

QString copyLines(const QString &text, const QSet<int> &lines)
{
    const SplitText split = splitLines(text);
    QList<int> order = lines.values();
    std::sort(order.begin(), order.end());
    QString out;
    for (int i : order) {
        if (i >= 0 && i < split.lines.size())
            out += split.lines.at(i) + split.eol;
    }
    return out;
}

QString replaceLines(const QString &text, const QSet<int> &lines, const QString &replacement)
{
    const QStringList with = splitLines(replacement).lines;
    return detail::mapLines(text, [&lines, &with](const QStringList &all) {
        QStringList out;
        for (int i = 0; i < all.size(); ++i) {
            if (lines.contains(i))
                out += with;
            else
                out.append(all.at(i));
        }
        return out;
    });
}

namespace {

// The match of the brace at pos, scanning with a depth count of that brace
// kind only, as the editor's bracket highlighting does.
BracePair braceAt(const QString &text, int pos)
{
    static const QString opens = QStringLiteral("([{");
    static const QString closes = QStringLiteral(")]}");
    if (pos < 0 || pos >= text.size())
        return {};
    const QChar ch = text.at(pos);
    const int kind = int(qMax(opens.indexOf(ch), closes.indexOf(ch)));
    if (kind < 0)
        return {};
    const bool forward = opens.contains(ch);
    const QChar partner = forward ? closes.at(kind) : opens.at(kind);
    int depth = 0;
    for (int i = pos; i >= 0 && i < text.size(); i += forward ? 1 : -1) {
        const QChar c = text.at(i);
        if (c == ch)
            ++depth;
        else if (c == partner)
            --depth;
        else
            continue;
        if (depth == 0)
            return {pos, i};
    }
    return {pos, -1};
}

} // namespace

BracePair matchingBrace(const QString &text, int pos)
{
    const BracePair here = braceAt(text, pos);
    return here.isValid() ? here : braceAt(text, pos - 1);
}

} // namespace TextTransforms

// Line-order transforms of TextTransforms (texttransforms.h), following the
// Notepad++ Line Operations sort commands.
#include "texttransforms.h"
#include "texttransforms_p.h"

#include <QCollator>
#include <QRandomGenerator>
#include <QRegularExpression>

#include <algorithm>
#include <optional>

namespace TextTransforms {

namespace {

using LessThan = std::function<bool(const QString &, const QString &)>;

// Stable, so lines that compare equal keep their order in both directions.
QString stableSort(const QString &text, SortOrder order, const LessThan &less)
{
    return detail::mapLines(text, [order, &less](QStringList lines) {
        if (order == SortOrder::Ascending)
            std::stable_sort(lines.begin(), lines.end(), less);
        else
            std::stable_sort(lines.begin(), lines.end(),
                             [&less](const QString &a, const QString &b) { return less(b, a); });
        return lines;
    });
}

// Lines with a key are sorted; the rest keep their order, first when
// ascending and last when descending (the Notepad++ placement).
template <typename Key>
QString keyedSort(const QString &text, SortOrder order,
                  const std::function<std::optional<Key>(const QString &)> &keyOf)
{
    return detail::mapLines(text, [order, &keyOf](const QStringList &lines) {
        QList<QPair<Key, QString>> keyed;
        QStringList others;
        for (const QString &line : lines) {
            if (const std::optional<Key> key = keyOf(line))
                keyed.append({*key, line});
            else
                others.append(line);
        }
        std::stable_sort(keyed.begin(), keyed.end(), [order](const auto &a, const auto &b) {
            return order == SortOrder::Ascending ? a.first < b.first : b.first < a.first;
        });
        QStringList sorted;
        for (const auto &entry : keyed)
            sorted.append(entry.second);
        return order == SortOrder::Ascending ? others + sorted : sorted + others;
    });
}

// An integer of any length: negative, then digit count, then digits.
struct IntegerKey {
    bool negative = false;
    QString digits;  // no leading zeros; empty for zero

    bool operator<(const IntegerKey &o) const
    {
        if (negative != o.negative)
            return negative;
        const bool magnitudeLess = digits.size() != o.digits.size()
                                       ? digits.size() < o.digits.size()
                                       : digits < o.digits;
        const bool magnitudeEqual = digits == o.digits;
        return negative ? (!magnitudeLess && !magnitudeEqual) : magnitudeLess;
    }
};

std::optional<IntegerKey> integerKey(const QString &line)
{
    static const QRegularExpression number(QStringLiteral("^[ \\t]*([+-]?)([0-9]+)"));
    const QRegularExpressionMatch m = number.match(line);
    if (!m.hasMatch())
        return std::nullopt;
    IntegerKey key;
    key.digits = m.captured(2);
    while (key.digits.startsWith(QLatin1Char('0')))
        key.digits.remove(0, 1);
    key.negative = m.captured(1) == QLatin1String("-") && !key.digits.isEmpty();
    return key;
}

std::optional<double> decimalKey(const QString &line, QChar separator)
{
    const QString sep = QRegularExpression::escape(QString(separator));
    const QRegularExpression number(
        QStringLiteral("^[ \\t]*([+-]?(?:[0-9]+(?:%1[0-9]*)?|%1[0-9]+))").arg(sep));
    const QRegularExpressionMatch m = number.match(line);
    if (!m.hasMatch())
        return std::nullopt;
    QString value = m.captured(1);
    value.replace(separator, QLatin1Char('.'));
    return value.toDouble();
}

} // namespace

QString sortLexicographic(const QString &text, SortOrder order, Qt::CaseSensitivity cs)
{
    return stableSort(text, order, [cs](const QString &a, const QString &b) {
        return QString::compare(a, b, cs) < 0;
    });
}

QString sortLocale(const QString &text, SortOrder order, const QLocale &locale)
{
    const QCollator collator(locale);
    return stableSort(text, order, [&collator](const QString &a, const QString &b) {
        return collator.compare(a, b) < 0;
    });
}

QString sortIntegers(const QString &text, SortOrder order)
{
    return keyedSort<IntegerKey>(text, order, integerKey);
}

QString sortDecimals(const QString &text, SortOrder order, QChar decimalSeparator)
{
    return keyedSort<double>(text, order, [decimalSeparator](const QString &line) {
        return decimalKey(line, decimalSeparator);
    });
}

QString sortByLength(const QString &text, SortOrder order)
{
    return stableSort(text, order, [](const QString &a, const QString &b) {
        return a.toUcs4().size() < b.toUcs4().size();
    });
}

QString reverseLines(const QString &text)
{
    return detail::mapLines(text, [](QStringList lines) {
        std::reverse(lines.begin(), lines.end());
        return lines;
    });
}

QString shuffleLines(const QString &text, quint32 seed)
{
    return detail::mapLines(text, [seed](QStringList lines) {
        QRandomGenerator generator(seed);
        std::shuffle(lines.begin(), lines.end(), generator);
        return lines;
    });
}

} // namespace TextTransforms

#ifndef TEXTTRANSFORMS_P_H
#define TEXTTRANSFORMS_P_H

// Helpers shared by the texttransforms*.cpp files. Not part of the API.

#include <QString>
#include <QStringList>
#include <functional>

namespace TextTransforms {
namespace detail {

// Notepad++ blank operations treat only space and tab as blanks. U+00A0 is
// text the user typed on purpose (spec D11), so it is never trimmed.
inline bool isBlank(QChar ch)
{
    return ch == QLatin1Char(' ') || ch == QLatin1Char('\t');
}

// Applies fn to the lines of text and rejoins them with the original EOL
// convention and trailing EOL (TextTransforms::splitLines).
QString mapLines(const QString &text, const std::function<QStringList(const QStringList &)> &fn);

// Applies fn to each line.
QString eachLine(const QString &text, const std::function<QString(const QString &)> &fn);

} // namespace detail
} // namespace TextTransforms

#endif // TEXTTRANSFORMS_P_H

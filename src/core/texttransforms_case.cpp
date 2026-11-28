// Case conversions of TextTransforms (texttransforms.h), following the
// Notepad++ Convert Case semantics.
#include "texttransforms.h"

#include <QRandomGenerator>

#include <vector>

namespace TextTransforms {

namespace {

// Code points of text. An unpaired surrogate is kept as its own value; it is
// not a letter, so every conversion passes it through untouched.
std::vector<char32_t> decode(const QString &text)
{
    std::vector<char32_t> out;
    out.reserve(size_t(text.size()));
    for (qsizetype i = 0; i < text.size(); ++i) {
        const QChar ch = text.at(i);
        if (ch.isHighSurrogate() && i + 1 < text.size() && text.at(i + 1).isLowSurrogate()) {
            out.push_back(QChar::surrogateToUcs4(ch, text.at(i + 1)));
            ++i;
        } else {
            out.push_back(ch.unicode());
        }
    }
    return out;
}

QString encode(const std::vector<char32_t> &points)
{
    QString out;
    out.reserve(qsizetype(points.size()));
    for (char32_t cp : points) {
        if (QChar::requiresSurrogates(cp)) {
            out.append(QChar(QChar::highSurrogate(cp)));
            out.append(QChar(QChar::lowSurrogate(cp)));
        } else {
            out.append(QChar(char16_t(cp)));
        }
    }
    return out;
}

// "don't": the t follows an apostrophe that follows a letter, so it continues
// the word rather than starting one.
bool startsWord(const std::vector<char32_t> &cps, size_t i)
{
    if (i == 0)
        return true;
    const char32_t prev = cps[i - 1];
    if (QChar::isLetterOrNumber(prev))
        return false;
    const bool apostrophe = prev == U'\'' || prev == U'’';
    return !(apostrophe && i >= 2 && QChar::isLetterOrNumber(cps[i - 2]));
}

bool isLineBreak(const std::vector<char32_t> &cps, size_t i)
{
    // CR LF counts once, at the LF
    return cps[i] == U'\n' || (cps[i] == U'\r' && (i + 1 >= cps.size() || cps[i + 1] != U'\n'));
}

} // namespace

QString properCase(const QString &text, bool blend)
{
    std::vector<char32_t> cps = decode(text);
    for (size_t i = 0; i < cps.size(); ++i) {
        if (!QChar::isLetter(cps[i]))
            continue;
        if (startsWord(cps, i))
            cps[i] = QChar::toUpper(cps[i]);
        else if (!blend)
            cps[i] = QChar::toLower(cps[i]);
    }
    return encode(cps);
}

QString sentenceCase(const QString &text, bool blend)
{
    std::vector<char32_t> cps = decode(text);
    bool newSentence = true;
    int lineBreaks = 0;  // consecutive, with only blanks between them
    for (size_t i = 0; i < cps.size(); ++i) {
        const char32_t cp = cps[i];
        if (QChar::isLetter(cp)) {
            cps[i] = newSentence ? QChar::toUpper(cp) : (blend ? cp : QChar::toLower(cp));
            newSentence = false;
            lineBreaks = 0;
        } else if (cp == U'.' || cp == U'!' || cp == U'?') {
            newSentence = true;
            lineBreaks = 0;
        } else if (isLineBreak(cps, i)) {
            if (++lineBreaks >= 2)
                newSentence = true;
        } else if (!QChar::isSpace(cp)) {
            lineBreaks = 0;
        }
    }
    return encode(cps);
}

QString invertCase(const QString &text)
{
    std::vector<char32_t> cps = decode(text);
    for (char32_t &cp : cps) {
        if (QChar::isUpper(cp))
            cp = QChar::toLower(cp);
        else if (QChar::isLower(cp))
            cp = QChar::toUpper(cp);
    }
    return encode(cps);
}

QString randomCase(const QString &text, quint32 seed)
{
    QRandomGenerator generator(seed);
    std::vector<char32_t> cps = decode(text);
    for (char32_t &cp : cps) {
        if (QChar::isLetter(cp))
            cp = generator.bounded(2) ? QChar::toUpper(cp) : QChar::toLower(cp);
    }
    return encode(cps);
}

} // namespace TextTransforms

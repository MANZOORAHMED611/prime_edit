#include "codectransforms.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <QStringDecoder>
#include <QUrl>

namespace CodecTransforms {

namespace {

QCryptographicHash::Algorithm qtAlgorithm(HashAlgorithm algorithm)
{
    switch (algorithm) {
    case HashAlgorithm::Md5: return QCryptographicHash::Md5;
    case HashAlgorithm::Sha1: return QCryptographicHash::Sha1;
    case HashAlgorithm::Sha256: return QCryptographicHash::Sha256;
    case HashAlgorithm::Sha512: break;
    }
    return QCryptographicHash::Sha512;
}

// Decoded bytes become text only when they are valid UTF-8.
std::optional<QString> fromUtf8Strict(const QByteArray &bytes)
{
    QStringDecoder decoder(QStringDecoder::Utf8, QStringDecoder::Flag::Stateless);
    const QString text = decoder.decode(bytes);
    if (decoder.hasError())
        return std::nullopt;
    return text;
}

QByteArray withoutWhitespace(const QString &text)
{
    QByteArray out;
    for (QChar ch : text) {
        if (!ch.isSpace())
            out.append(ch.unicode() < 0x80 ? char(ch.unicode()) : '\x80');  // non-ASCII is invalid
    }
    return out;
}

int hexValue(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

} // namespace

QString hashText(const QString &text, HashAlgorithm algorithm)
{
    return QString::fromLatin1(QCryptographicHash::hash(text.toUtf8(), qtAlgorithm(algorithm)).toHex());
}

std::optional<QString> hashFile(const QString &path, HashAlgorithm algorithm)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return std::nullopt;
    QCryptographicHash hash(qtAlgorithm(algorithm));
    if (!hash.addData(&file))
        return std::nullopt;
    return QString::fromLatin1(hash.result().toHex());
}

QString base64Encode(const QString &text)
{
    return QString::fromLatin1(text.toUtf8().toBase64());
}

std::optional<QString> base64Decode(const QString &text)
{
    const auto decoded = QByteArray::fromBase64Encoding(withoutWhitespace(text),
                                                        QByteArray::AbortOnBase64DecodingErrors);
    if (!decoded)
        return std::nullopt;
    return fromUtf8Strict(*decoded);
}

QString urlEncode(const QString &text)
{
    return QString::fromLatin1(QUrl::toPercentEncoding(text));
}

std::optional<QString> urlDecode(const QString &text)
{
    const QByteArray bytes = text.toUtf8();
    QByteArray out;
    for (qsizetype i = 0; i < bytes.size(); ++i) {
        if (bytes.at(i) != '%') {
            out.append(bytes.at(i));
            continue;
        }
        const int high = i + 2 < bytes.size() ? hexValue(bytes.at(i + 1)) : -1;
        const int low = high >= 0 ? hexValue(bytes.at(i + 2)) : -1;
        if (low < 0)
            return std::nullopt;
        out.append(char(high * 16 + low));
        i += 2;
    }
    return fromUtf8Strict(out);
}

QString asciiToHex(const QString &text)
{
    return QString::fromLatin1(text.toUtf8().toHex()).toUpper();
}

std::optional<QString> hexToAscii(const QString &text)
{
    const QByteArray digits = withoutWhitespace(text);
    if (digits.size() % 2)
        return std::nullopt;
    for (char c : digits) {
        if (hexValue(c) < 0)
            return std::nullopt;
    }
    return fromUtf8Strict(QByteArray::fromHex(digits));
}

} // namespace CodecTransforms

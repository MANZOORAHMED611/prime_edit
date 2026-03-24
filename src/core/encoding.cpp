#include "encoding.h"
#include <QStringEncoder>
#include <QStringDecoder>

bool Encoding::s_initialized = false;

QStringList Encoding::availableEncodings()
{
    initEncodings();
    QStringList result;
    for (const auto &info : encodings()) {
        result.append(info.displayName);
    }
    return result;
}

QString Encoding::displayName(const QString &encoding)
{
    initEncodings();
    if (encodings().contains(encoding)) {
        return encodings()[encoding].displayName;
    }
    return encoding;
}

QString Encoding::codecName(const QString &displayName)
{
    initEncodings();
    for (auto it = encodings().begin(); it != encodings().end(); ++it) {
        if (it.value().displayName == displayName) {
            return it.key();
        }
    }
    return displayName;
}

bool Encoding::isValid(const QString &encoding)
{
    initEncodings();
    return encodings().contains(encoding);
}

QByteArray Encoding::encode(const QString &text, const QString &encoding)
{
    if (encoding == "UTF-8" || encoding == "utf8") {
        return text.toUtf8();
    }
    if (encoding == "UTF-8-BOM") {
        return QByteArray("\xEF\xBB\xBF") + text.toUtf8();
    }
    if (encoding == "UTF-16LE") {
        auto encoder = QStringEncoder(QStringEncoder::Utf16LE);
        return encoder(text);
    }
    if (encoding == "UTF-16BE") {
        auto encoder = QStringEncoder(QStringEncoder::Utf16BE);
        return encoder(text);
    }
    if (encoding == "ISO-8859-1" || encoding == "Latin1") {
        return text.toLatin1();
    }

    // Default to UTF-8
    return text.toUtf8();
}

QString Encoding::decode(const QByteArray &data, const QString &encoding)
{
    if (encoding == "UTF-8" || encoding == "utf8" || encoding == "UTF-8-BOM") {
        QByteArray actualData = data;
        if (data.startsWith("\xEF\xBB\xBF")) {
            actualData = data.mid(3);
        }
        return QString::fromUtf8(actualData);
    }
    if (encoding == "UTF-16LE") {
        auto decoder = QStringDecoder(QStringDecoder::Utf16LE);
        return decoder(data);
    }
    if (encoding == "UTF-16BE") {
        auto decoder = QStringDecoder(QStringDecoder::Utf16BE);
        return decoder(data);
    }
    if (encoding == "ISO-8859-1" || encoding == "Latin1") {
        return QString::fromLatin1(data);
    }

    // Default to UTF-8
    return QString::fromUtf8(data);
}

QMap<QString, Encoding::EncodingInfo> &Encoding::encodings()
{
    static QMap<QString, EncodingInfo> map;
    return map;
}

void Encoding::initEncodings()
{
    if (s_initialized) return;
    s_initialized = true;

    auto &map = encodings();

    map["UTF-8"] = {"UTF-8", "UTF-8", {"utf8", "utf-8"}};
    map["UTF-8-BOM"] = {"UTF-8-BOM", "UTF-8 with BOM", {"utf8-bom"}};
    map["UTF-16LE"] = {"UTF-16LE", "UTF-16 LE", {"utf-16le", "utf16le"}};
    map["UTF-16BE"] = {"UTF-16BE", "UTF-16 BE", {"utf-16be", "utf16be"}};
    map["ISO-8859-1"] = {"ISO-8859-1", "ISO-8859-1 (Latin-1)", {"latin1", "iso8859-1"}};
    map["ISO-8859-2"] = {"ISO-8859-2", "ISO-8859-2 (Latin-2)", {"latin2", "iso8859-2"}};
    map["ISO-8859-15"] = {"ISO-8859-15", "ISO-8859-15 (Latin-9)", {"latin9", "iso8859-15"}};
    map["Windows-1252"] = {"Windows-1252", "Windows-1252", {"cp1252"}};
    map["ASCII"] = {"ASCII", "ASCII", {"us-ascii"}};
}

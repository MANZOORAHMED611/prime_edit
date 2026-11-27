#include "textcodec.h"

#include <QStringDecoder>
#include <QStringEncoder>

#include <optional>
#include <unicode/ucnv.h>
#include <unicode/ucnv_cb.h>

// UTF-8/16/32 go through QStringConverter; every other charset through ICU
// ucnv. Fallback mappings stay off (the ICU default), so a many-to-one charset
// shows up as a failed round trip instead of being silently normalised.

namespace {

const QString kBomSuffix = QStringLiteral("-BOM");

struct ParsedName {
    QString base;
    bool bom = false;
};

ParsedName parseName(const QString &name)
{
    if (name.endsWith(kBomSuffix, Qt::CaseInsensitive))
        return {name.chopped(kBomSuffix.size()), true};
    return {name, false};
}

struct UtfCodec {
    const char *name;
    QStringConverter::Encoding encoding;
    QByteArray bom;
};

std::optional<UtfCodec> utfCodec(const QString &base)
{
    static const UtfCodec codecs[] = {
        {"UTF-8", QStringConverter::Utf8, QByteArray("\xEF\xBB\xBF", 3)},
        {"UTF-16LE", QStringConverter::Utf16LE, QByteArray("\xFF\xFE", 2)},
        {"UTF-16BE", QStringConverter::Utf16BE, QByteArray("\xFE\xFF", 2)},
        {"UTF-32LE", QStringConverter::Utf32LE, QByteArray("\xFF\xFE\x00\x00", 4)},
        {"UTF-32BE", QStringConverter::Utf32BE, QByteArray("\x00\x00\xFE\xFF", 4)},
    };
    for (const UtfCodec &codec : codecs) {
        if (base.compare(QLatin1String(codec.name), Qt::CaseInsensitive) == 0)
            return codec;
    }
    return std::nullopt;
}

// Closes an ICU converter on scope exit.
struct Converter {
    explicit Converter(const QString &name)
    {
        UErrorCode status = U_ZERO_ERROR;
        handle = ucnv_open(name.toLatin1().constData(), &status);
        if (U_FAILURE(status))
            handle = nullptr;
    }
    ~Converter()
    {
        if (handle)
            ucnv_close(handle);
    }
    Converter(const Converter &) = delete;
    Converter &operator=(const Converter &) = delete;
    UConverter *handle = nullptr;
};

bool isConversionError(UConverterCallbackReason reason)
{
    return reason == UCNV_UNASSIGNED || reason == UCNV_ILLEGAL || reason == UCNV_IRREGULAR;
}

void U_CALLCONV recordDecodeError(const void *context, UConverterToUnicodeArgs *args,
                                  const char *codeUnits, int32_t length,
                                  UConverterCallbackReason reason, UErrorCode *status)
{
    if (isConversionError(reason))
        *static_cast<bool *>(const_cast<void *>(context)) = true;
    UCNV_TO_U_CALLBACK_SUBSTITUTE(nullptr, args, codeUnits, length, reason, status);
}

struct EncodeContext {
    const UChar *start;
    QList<int> *unmappable;
};

// ICU calls this after consuming the offending code units, so the character
// starts `length` units before the current source position.
void U_CALLCONV recordEncodeError(const void *context, UConverterFromUnicodeArgs *args,
                                  const UChar *codeUnits, int32_t length, UChar32 codePoint,
                                  UConverterCallbackReason reason, UErrorCode *status)
{
    if (isConversionError(reason)) {
        auto *ctx = static_cast<EncodeContext *>(const_cast<void *>(context));
        ctx->unmappable->append(static_cast<int>(args->source - ctx->start) - length);
    }
    UCNV_FROM_U_CALLBACK_SUBSTITUTE(nullptr, args, codeUnits, length, codePoint, reason, status);
}

DecodeResult decodeIcu(const QByteArray &bytes, const QString &name)
{
    Converter conv(name);
    if (!conv.handle)
        return {QString(), false};
    bool error = false;
    UErrorCode status = U_ZERO_ERROR;
    ucnv_setToUCallBack(conv.handle, recordDecodeError, &error, nullptr, nullptr, &status);
    QString text(bytes.size() * 2 + 2, Qt::Uninitialized);
    UChar *target = reinterpret_cast<UChar *>(text.data());
    const UChar *targetLimit = target + text.size();
    const char *source = bytes.constData();
    ucnv_toUnicode(conv.handle, &target, targetLimit, &source, source + bytes.size(),
                   nullptr, true, &status);
    if (U_FAILURE(status))
        return {QString(), false};
    text.truncate(static_cast<qsizetype>(target - reinterpret_cast<UChar *>(text.data())));
    return {text, !error};
}

// Used when the converter cannot run at all: report every character, so the
// caller refuses the write instead of saving partial or empty bytes.
EncodeResult refuseAll(const QString &text)
{
    EncodeResult result;
    for (int i = 0; i < text.size(); ++i)
        result.unmappable.append(i);
    return result;
}

EncodeResult encodeIcu(const QString &text, const QString &name)
{
    EncodeResult result;
    Converter conv(name);
    if (!conv.handle)
        return refuseAll(text);
    const UChar *start = reinterpret_cast<const UChar *>(text.utf16());
    EncodeContext context{start, &result.unmappable};
    UErrorCode status = U_ZERO_ERROR;
    ucnv_setFromUCallBack(conv.handle, recordEncodeError, &context, nullptr, nullptr, &status);
    result.bytes.resize(UCNV_GET_MAX_BYTES_FOR_STRING(text.size(), ucnv_getMaxCharSize(conv.handle)));
    char *target = result.bytes.data();
    const UChar *source = start;
    ucnv_fromUnicode(conv.handle, &target, target + result.bytes.size(), &source,
                     start + text.size(), nullptr, true, &status);
    if (U_FAILURE(status))
        return refuseAll(text);
    result.bytes.truncate(static_cast<qsizetype>(target - result.bytes.data()));
    return result;
}

// The only characters a UTF encoding cannot represent are lone surrogates.
QList<int> loneSurrogates(const QString &text)
{
    QList<int> positions;
    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (c.isHighSurrogate() && i + 1 < text.size() && text.at(i + 1).isLowSurrogate())
            ++i;
        else if (c.isSurrogate())
            positions.append(i);
    }
    return positions;
}

} // namespace

namespace TextCodec {

DecodeResult decode(const QByteArray &bytes, const QString &name)
{
    const ParsedName parsed = parseName(name);
    DecodeResult result;
    if (const auto utf = utfCodec(parsed.base)) {
        const bool stripBom = parsed.bom && bytes.startsWith(utf->bom);
        // A BOM the name does not declare is kept as U+FEFF, never dropped.
        QStringDecoder decoder(utf->encoding, QStringConverter::Flag::ConvertInitialBom);
        result.text = decoder(stripBom ? bytes.mid(utf->bom.size()) : bytes);
        result.lossless = !decoder.hasError();
    } else {
        result = decodeIcu(bytes, parsed.base);
    }
    if (result.lossless) {
        const EncodeResult back = encode(result.text, name);
        result.lossless = back.unmappable.isEmpty() && back.bytes == bytes;
    }
    return result;
}

EncodeResult encode(const QString &text, const QString &name)
{
    const ParsedName parsed = parseName(name);
    const auto utf = utfCodec(parsed.base);
    if (!utf)
        return encodeIcu(text, parsed.base);
    EncodeResult result;
    result.unmappable = loneSurrogates(text);
    QStringEncoder encoder(utf->encoding);
    result.bytes = (parsed.bom ? utf->bom : QByteArray()) + QByteArray(encoder(text));
    return result;
}

QString canonicalName(const QString &name)
{
    const ParsedName parsed = parseName(name);
    const QString suffix = parsed.bom ? kBomSuffix : QString();
    if (const auto utf = utfCodec(parsed.base))
        return QLatin1String(utf->name) + suffix;
    UErrorCode status = U_ZERO_ERROR;
    const char *converter = ucnv_getAlias(parsed.base.toLatin1().constData(), 0, &status);
    if (U_FAILURE(status) || !converter)
        return QString();
    // MIME first, then IANA: this gives the spellings of the spec 9.1 table
    // (windows-1252, Shift_JIS, ISO-8859-1, ISO-8859-6, windows-1256).
    for (const char *standard : {"MIME", "IANA"}) {
        UErrorCode lookup = U_ZERO_ERROR;
        const char *standardName = ucnv_getStandardName(converter, standard, &lookup);
        if (U_SUCCESS(lookup) && standardName)
            return QLatin1String(standardName) + suffix;
    }
    return QLatin1String(converter) + suffix;
}

} // namespace TextCodec

#include "textcodec.h"

// Placeholder (spec R2-1) until T1.3 implements the codec. Nothing calls these
// at the W0 base. They report the most conservative answer: no decode is
// lossless and no character is encodable, so an early caller can never treat a
// lossy conversion as safe.

namespace TextCodec {

DecodeResult decode(const QByteArray &bytes, const QString &name)
{
    Q_UNUSED(bytes);
    Q_UNUSED(name);
    return DecodeResult{QString(), false};
}

EncodeResult encode(const QString &text, const QString &name)
{
    Q_UNUSED(name);
    EncodeResult result;
    result.unmappable.reserve(text.size());
    for (int i = 0; i < text.size(); ++i) {
        result.unmappable.append(i);
    }
    return result;
}

} // namespace TextCodec

#ifndef TEXTCODEC_H
#define TEXTCODEC_H

#include <QByteArray>
#include <QList>
#include <QString>

// lossless is true only when decoding raised no error and re-encoding the text
// gives back the exact input bytes, so a many-to-one codec cannot pass silently.
struct DecodeResult {
    QString text;
    bool lossless = false;
};

// unmappable holds QString (UTF-16) indices of characters the target encoding
// cannot represent. A non-empty list means writing bytes would lose text.
struct EncodeResult {
    QByteArray bytes;
    QList<int> unmappable;
};

// The single byte/text converter for the load and save paths. Stateless, so it
// is a namespace rather than a QObject. Encoding names follow spec section 9.1.
namespace TextCodec {
DecodeResult decode(const QByteArray &bytes, const QString &name);
EncodeResult encode(const QString &text, const QString &name);
}

#endif // TEXTCODEC_H

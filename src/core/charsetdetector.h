#ifndef CHARSETDETECTOR_H
#define CHARSETDETECTOR_H

#include <QString>
#include <QByteArray>

// name uses the spec 9.1 table spelling without a "-BOM" suffix; hasBom says
// whether the bytes start with that encoding's byte-order mark.
struct CharsetGuess {
    QString name;
    bool hasBom = false;
};

class CharsetDetector
{
public:
    // Examines every byte it is given. Document passes the whole file, or an
    // 8 KB probe for a partial view.
    static CharsetGuess detectCharset(const QByteArray &data);

    // 1.0.0 entry point, kept because the frozen validation suites call it:
    // the same guess as one string, "-BOM" appended only for UTF-8 as in 1.0.0.
    static QString detect(const QByteArray &data);

private:
    static QString detectWithoutBom(const unsigned char *bytes, qsizetype len);
    static bool isValidUtf8(const unsigned char *data, qsizetype length);
    static qsizetype countHighBytes(
        const unsigned char *data, qsizetype length,
        int rangeStart, int rangeEnd);
};

#endif // CHARSETDETECTOR_H

#ifndef CHARSETDETECTOR_H
#define CHARSETDETECTOR_H

#include <QString>
#include <QByteArray>

class CharsetDetector
{
public:
    static QString detect(const QByteArray &data, int maxBytes = 8192);

private:
    static bool isValidUtf8(const unsigned char *data, int length);
    static int countHighBytes(
        const unsigned char *data, int length,
        int rangeStart, int rangeEnd);
};

#endif // CHARSETDETECTOR_H

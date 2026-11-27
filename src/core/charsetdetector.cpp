#include "charsetdetector.h"

QString CharsetDetector::detect(const QByteArray &data)
{
    const CharsetGuess guess = detectCharset(data);
    if (guess.hasBom && guess.name == QLatin1String("UTF-8"))
        return QStringLiteral("UTF-8-BOM");
    return guess.name;
}

CharsetGuess CharsetDetector::detectCharset(const QByteArray &data)
{
    const unsigned char *bytes =
        reinterpret_cast<const unsigned char *>(data.constData());
    const qsizetype len = data.size();

    // BOM markers first. UTF-32LE before UTF-16LE: both start with FF FE.
    if (len >= 4 && bytes[0] == 0xFF && bytes[1] == 0xFE
        && bytes[2] == 0x00 && bytes[3] == 0x00) {
        return {"UTF-32LE", true};
    }
    if (len >= 4 && bytes[0] == 0x00 && bytes[1] == 0x00
        && bytes[2] == 0xFE && bytes[3] == 0xFF) {
        return {"UTF-32BE", true};
    }
    if (len >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB
        && bytes[2] == 0xBF) {
        return {"UTF-8", true};
    }
    if (len >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE) {
        return {"UTF-16LE", true};
    }
    if (len >= 2 && bytes[0] == 0xFE && bytes[1] == 0xFF) {
        return {"UTF-16BE", true};
    }
    return {detectWithoutBom(bytes, len), false};
}

// The 1.0.0 heuristics, unchanged except that they see every byte they are
// given (1.0.0 stopped at 8 KB) and return the spec 9.1 spellings. A wrong
// guess is caught by the lossless check on load, which opens the file
// read-only and offers Reopen with Encoding.
QString CharsetDetector::detectWithoutBom(const unsigned char *bytes, qsizetype len)
{
    if (isValidUtf8(bytes, len)) {
        return "UTF-8";
    }

    const qsizetype highBytes = countHighBytes(bytes, len, 0x80, 0xFF);

    // Windows-1252: bytes in 0x80-0x9F are control chars in
    // ISO-8859-1 but printable in Windows-1252
    if (countHighBytes(bytes, len, 0x80, 0x9F) > 0) {
        return "windows-1252";
    }

    // Shift-JIS: lead bytes 0x81-0x9F and 0xE0-0xEF
    qsizetype sjisLeads = 0;
    for (qsizetype i = 0; i < len - 1; ++i) {
        if ((bytes[i] >= 0x81 && bytes[i] <= 0x9F)
            || (bytes[i] >= 0xE0 && bytes[i] <= 0xEF)) {
            if (bytes[i + 1] >= 0x40 && bytes[i + 1] <= 0xFC
                && bytes[i + 1] != 0x7F) {
                sjisLeads++;
                i++; // skip trail byte
            }
        }
    }
    if (sjisLeads > highBytes / 3) {
        return "Shift_JIS";
    }

    return "ISO-8859-1";
}

bool CharsetDetector::isValidUtf8(
    const unsigned char *data, qsizetype length)
{
    for (qsizetype i = 0; i < length;) {
        if (data[i] < 0x80) {
            i++;
            continue;
        }

        int seqLen;
        if ((data[i] & 0xE0) == 0xC0) {
            seqLen = 2;
        } else if ((data[i] & 0xF0) == 0xE0) {
            seqLen = 3;
        } else if ((data[i] & 0xF8) == 0xF0) {
            seqLen = 4;
        } else {
            return false;
        }

        if (i + seqLen > length) {
            return false;
        }
        for (int j = 1; j < seqLen; j++) {
            if ((data[i + j] & 0xC0) != 0x80) {
                return false;
            }
        }
        i += seqLen;
    }
    return true;
}

qsizetype CharsetDetector::countHighBytes(
    const unsigned char *data, qsizetype length,
    int rangeStart, int rangeEnd)
{
    qsizetype count = 0;
    for (qsizetype i = 0; i < length; ++i) {
        if (data[i] >= rangeStart && data[i] <= rangeEnd) {
            count++;
        }
    }
    return count;
}

#include "charsetdetector.h"

QString CharsetDetector::detect(const QByteArray &data, int maxBytes)
{
    if (data.isEmpty()) {
        return "UTF-8";
    }

    const unsigned char *bytes =
        reinterpret_cast<const unsigned char *>(data.constData());
    int len = qMin(data.size(), maxBytes);

    // Check BOM markers first
    if (len >= 4 && bytes[0] == 0xFF && bytes[1] == 0xFE
        && bytes[2] == 0x00 && bytes[3] == 0x00) {
        return "UTF-32LE";
    }
    if (len >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB
        && bytes[2] == 0xBF) {
        return "UTF-8-BOM";
    }
    if (len >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE) {
        return "UTF-16LE";
    }
    if (len >= 2 && bytes[0] == 0xFE && bytes[1] == 0xFF) {
        return "UTF-16BE";
    }

    // Check if valid UTF-8
    if (isValidUtf8(bytes, len)) {
        return "UTF-8";
    }

    // Heuristic: count bytes in high ranges
    int highBytes = countHighBytes(bytes, len, 0x80, 0xFF);
    if (highBytes == 0) {
        return "UTF-8"; // Pure ASCII
    }

    // Windows-1252: bytes in 0x80-0x9F are control chars in
    // ISO-8859-1 but printable in Windows-1252
    int win1252Range = countHighBytes(bytes, len, 0x80, 0x9F);
    if (win1252Range > 0) {
        return "Windows-1252";
    }

    // Shift-JIS: lead bytes 0x81-0x9F and 0xE0-0xEF
    int sjisLeads = 0;
    for (int i = 0; i < len - 1; ++i) {
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
        return "Shift-JIS";
    }

    // Default fallback
    return "ISO-8859-1";
}

bool CharsetDetector::isValidUtf8(
    const unsigned char *data, int length)
{
    for (int i = 0; i < length;) {
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

int CharsetDetector::countHighBytes(
    const unsigned char *data, int length,
    int rangeStart, int rangeEnd)
{
    int count = 0;
    for (int i = 0; i < length; ++i) {
        if (data[i] >= rangeStart && data[i] <= rangeEnd) {
            count++;
        }
    }
    return count;
}

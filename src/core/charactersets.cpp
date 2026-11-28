#include "charactersets.h"

#include "textcodec.h"

namespace CharacterSets {

namespace {

struct Entry {
    const char *region;
    const char *label;
    const char *encoding;
};

// Notepad++'s list. OEM code pages use ICU's cpNNN aliases, except OEM 720,
// which ICU 70 knows only as ibm-720.
const Entry kEntries[] = {
    {"Arabic", "ISO 8859-6", "ISO-8859-6"}, {"Arabic", "OEM 720", "ibm-720"},
    {"Arabic", "Windows-1256", "windows-1256"},
    {"Baltic", "ISO 8859-4", "ISO-8859-4"}, {"Baltic", "ISO 8859-13", "ISO-8859-13"},
    {"Baltic", "OEM 775", "cp775"}, {"Baltic", "Windows-1257", "windows-1257"},
    {"Celtic", "ISO 8859-14", "ISO-8859-14"},
    {"Cyrillic", "ISO 8859-5", "ISO-8859-5"}, {"Cyrillic", "KOI8-R", "KOI8-R"},
    {"Cyrillic", "KOI8-U", "KOI8-U"}, {"Cyrillic", "Macintosh", "x-mac-cyrillic"},
    {"Cyrillic", "OEM 855", "cp855"}, {"Cyrillic", "OEM 866", "cp866"},
    {"Cyrillic", "Windows-1251", "windows-1251"},
    {"Central European", "OEM 852", "cp852"}, {"Central European", "Windows-1250", "windows-1250"},
    {"Chinese", "Big5 (Traditional)", "Big5"}, {"Chinese", "GB2312 (Simplified)", "GB2312"},
    {"Eastern European", "ISO 8859-2", "ISO-8859-2"},
    {"Greek", "ISO 8859-7", "ISO-8859-7"}, {"Greek", "OEM 737", "cp737"},
    {"Greek", "OEM 869", "cp869"}, {"Greek", "Windows-1253", "windows-1253"},
    {"Hebrew", "ISO 8859-8", "ISO-8859-8"}, {"Hebrew", "OEM 862", "cp862"},
    {"Hebrew", "Windows-1255", "windows-1255"},
    {"Japanese", "Shift-JIS", "Shift_JIS"},
    {"Korean", "Windows 949", "windows-949"}, {"Korean", "EUC-KR", "EUC-KR"},
    {"North European", "OEM 861 : Icelandic", "cp861"}, {"North European", "OEM 865 : Nordic", "cp865"},
    {"Thai", "TIS-620", "TIS-620"},
    {"Turkish", "ISO 8859-3", "ISO-8859-3"}, {"Turkish", "ISO 8859-9", "ISO-8859-9"},
    {"Turkish", "OEM 857", "cp857"}, {"Turkish", "Windows-1254", "windows-1254"},
    {"Western European", "ISO 8859-1", "ISO-8859-1"}, {"Western European", "ISO 8859-15", "ISO-8859-15"},
    {"Western European", "OEM 850", "cp850"}, {"Western European", "OEM 858", "cp858"},
    {"Western European", "OEM 860", "cp860"}, {"Western European", "OEM 863", "cp863"},
    {"Western European", "OEM-US", "cp437"}, {"Western European", "Windows-1252", "windows-1252"},
    {"Vietnamese", "Windows-1258", "windows-1258"},
};

} // namespace

QList<Group> available()
{
    QList<Group> groups;
    for (const Entry &entry : kEntries) {
        const QString encoding = QString::fromLatin1(entry.encoding);
        if (TextCodec::canonicalName(encoding).isEmpty())
            continue;
        const QString region = QString::fromLatin1(entry.region);
        if (groups.isEmpty() || groups.constLast().region != region)
            groups.append({region, {}});
        groups.last().sets.append({QString::fromLatin1(entry.label), encoding});
    }
    return groups;
}

} // namespace CharacterSets

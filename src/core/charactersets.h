#ifndef CHARACTERSETS_H
#define CHARACTERSETS_H

#include <QList>
#include <QString>

// The Notepad++ Encoding > Character Sets list: legacy code pages grouped by
// region, in Notepad++'s order and wording. Only code pages TextCodec can
// convert (TextCodec::canonicalName is not empty) are returned, so the menu
// never offers one that would fail.
namespace CharacterSets {

struct Set {
    QString label;     // menu text, e.g. "Windows-1256"
    QString encoding;  // a name TextCodec accepts, e.g. "windows-1256"
};

struct Group {
    QString region;
    QList<Set> sets;
};

QList<Group> available();

} // namespace CharacterSets

#endif // CHARACTERSETS_H

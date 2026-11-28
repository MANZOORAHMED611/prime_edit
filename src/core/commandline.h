#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include "openoptions.h"

#include <QStringList>

class QCommandLineParser;

struct LaunchRequest {
    QStringList files;  // absolute, made so against this process's cwd
    OpenOptions options;
};

// The command line, in both PrimeEdit and Notepad++ syntax:
//   -g N, --goto N, -n N, --line=N, -nN      go to line N
//   -c N, --column=N, -cN                    and to column N of it
//   -p N, --position=N, -pN                  or to character offset N (0-based)
//   -l NAME, --language=NAME, -lNAME         set the language
//   --read-only, -ro                         open read-only
namespace CommandLine {

// Rewrites Notepad++'s attached forms (-n12, -c5, -p100, -lpython, -ro) into
// the long options QCommandLineParser understands. The program name and
// anything after "--" are left alone, as is anything that does not match.
QStringList normalizeNppArguments(const QStringList &arguments);

// Adds the file arguments and the options above (not --help or --version).
void addOptions(QCommandLineParser &parser);

// Reads a parsed parser. Out-of-range or non-numeric values count as absent.
LaunchRequest request(const QCommandLineParser &parser);

} // namespace CommandLine

#endif // COMMANDLINE_H

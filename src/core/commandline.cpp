#include "commandline.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>

namespace CommandLine {

namespace {

// value as an int of at least `minimum`, or `absent`.
int intValue(const QCommandLineParser &parser, const QString &name, int minimum, int absent)
{
    bool ok = false;
    const int value = parser.value(name).toInt(&ok);
    return ok && value >= minimum ? value : absent;
}

} // namespace

QStringList normalizeNppArguments(const QStringList &arguments)
{
    static const QRegularExpression numeric(QStringLiteral("^-([ncp])([0-9]+)$"));
    static const QRegularExpression language(QStringLiteral("^-l([A-Za-z0-9_.+#-]+)$"));
    static const QHash<QString, QString> longNames = {
        {QStringLiteral("n"), QStringLiteral("line")},
        {QStringLiteral("c"), QStringLiteral("column")},
        {QStringLiteral("p"), QStringLiteral("position")}};
    QStringList out;
    bool optionsEnded = false;
    for (int i = 0; i < arguments.size(); ++i) {
        const QString &arg = arguments.at(i);
        optionsEnded = optionsEnded || arg == QLatin1String("--");
        const QRegularExpressionMatch number = numeric.match(arg);
        const QRegularExpressionMatch name = language.match(arg);
        if (i == 0 || optionsEnded)
            out << arg;
        else if (number.hasMatch())
            out << QStringLiteral("--%1=%2").arg(longNames.value(number.captured(1)), number.captured(2));
        else if (name.hasMatch())
            out << QStringLiteral("--language=") + name.captured(1);
        else if (arg == QLatin1String("-ro"))
            out << QStringLiteral("--read-only");
        else
            out << arg;
    }
    return out;
}

void addOptions(QCommandLineParser &parser)
{
    parser.addPositionalArgument(QStringLiteral("files"), QStringLiteral("Files to open"),
                                 QStringLiteral("[files...]"));
    // Forwarded like any other launch until one process supports several
    // windows (spec Q2).
    parser.addOption({QStringLiteral("new-window"), QStringLiteral("Open in a new window")});
    parser.addOption({{QStringLiteral("g"), QStringLiteral("goto"), QStringLiteral("n"), QStringLiteral("line")},
                      QStringLiteral("Go to line number (also -nN)"), QStringLiteral("line")});
    parser.addOption({{QStringLiteral("c"), QStringLiteral("column")},
                      QStringLiteral("Go to this column of the line (also -cN)"), QStringLiteral("column")});
    parser.addOption({{QStringLiteral("p"), QStringLiteral("position")},
                      QStringLiteral("Go to this 0-based character offset when no line is given (also -pN)"),
                      QStringLiteral("offset")});
    parser.addOption({{QStringLiteral("l"), QStringLiteral("language")},
                      QStringLiteral("Set the language, e.g. python or cpp (also -lNAME)"),
                      QStringLiteral("name")});
    parser.addOption({QStringLiteral("read-only"), QStringLiteral("Open the files read-only (also -ro)")});
}

LaunchRequest request(const QCommandLineParser &parser)
{
    LaunchRequest request;
    for (const QString &file : parser.positionalArguments())
        request.files.append(QFileInfo(file).absoluteFilePath());
    request.options.line = intValue(parser, QStringLiteral("line"), 1, 0);
    request.options.column = intValue(parser, QStringLiteral("column"), 1, 0);
    request.options.position = intValue(parser, QStringLiteral("position"), 0, -1);
    request.options.language = parser.value(QStringLiteral("language"));
    request.options.readOnly = parser.isSet(QStringLiteral("read-only"));
    return request;
}

} // namespace CommandLine

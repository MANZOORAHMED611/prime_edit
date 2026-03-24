#ifndef ENCODING_H
#define ENCODING_H

#include <QString>
#include <QStringList>
#include <QMap>

class Encoding
{
public:
    struct EncodingInfo {
        QString name;
        QString displayName;
        QStringList aliases;
    };

    static QStringList availableEncodings();
    static QString displayName(const QString &encoding);
    static QString codecName(const QString &displayName);
    static bool isValid(const QString &encoding);

    static QByteArray encode(const QString &text, const QString &encoding);
    static QString decode(const QByteArray &data, const QString &encoding);

private:
    static QMap<QString, EncodingInfo> &encodings();
    static void initEncodings();
    static bool s_initialized;
};

#endif // ENCODING_H

#ifndef LANGUAGEMANAGER_H
#define LANGUAGEMANAGER_H

#include <QString>
#include <QStringList>
#include <QMap>

class LanguageManager
{
public:
    static LanguageManager &instance();

    QString languageForExtension(const QString &extension) const;
    QString languageForFilename(const QString &filename) const;
    QString languageForShebang(const QString &shebang) const;

    QStringList availableLanguages() const;
    QStringList extensionsForLanguage(const QString &language) const;

    QString detectLanguage(const QString &filename, const QString &content) const;
    QString definitionPath(const QString &language) const;

private:
    LanguageManager();
    void initMappings();

    QMap<QString, QString> m_extensionToLanguage;
    QMap<QString, QStringList> m_languageToExtensions;
    QMap<QString, QString> m_filenameToLanguage;
    QMap<QString, QString> m_shebangToLanguage;
};

#endif // LANGUAGEMANAGER_H

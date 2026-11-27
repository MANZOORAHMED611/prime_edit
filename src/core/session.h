#ifndef SESSION_H
#define SESSION_H

#include <QString>
#include <QJsonObject>
#include <QStringList>

class MainWindow;

class Session
{
public:
    static Session &instance();

    void save(MainWindow *window);
    void restore(MainWindow *window);

    // Named sessions
    QStringList savedSessions() const;
    void saveSession(const QString &name, MainWindow *window);
    void loadSession(const QString &name, MainWindow *window);
    void deleteSession(const QString &name);

    // Unsaved document persistence
    void saveUnsavedDocuments(MainWindow *window);
    void restoreUnsavedDocuments(MainWindow *window);
    void clearUnsavedCache();

private:
    Session();

    QString sessionFilePath() const;
    QString namedSessionPath(const QString &name) const;
    QString unsavedCacheDir() const;
    QJsonObject windowToJson(MainWindow *window) const;
    void jsonToWindow(const QJsonObject &json, MainWindow *window);
    QJsonObject readSessionFile(const QString &path, bool *corrupt) const;

    // Set when restore() found session.json unparsable (spec D10).
    bool m_holdSessionWrites = false;
    QJsonObject m_heldSession;
};

#endif // SESSION_H

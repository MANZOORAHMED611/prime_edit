#include "session.h"
#include "ui/mainwindow.h"
#include "ui/editor.h"
#include "ui/tabwidget.h"
#include "core/document.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>

Session &Session::instance()
{
    static Session instance;
    return instance;
}

Session::Session()
{
}

void Session::save(MainWindow *window)
{
    QJsonObject json = windowToJson(window);

    QFile file(sessionFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    QJsonDocument doc(json);
    file.write(doc.toJson());
}

void Session::restore(MainWindow *window)
{
    QFile file(sessionFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) {
        return;
    }

    jsonToWindow(doc.object(), window);
}

QStringList Session::savedSessions() const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sessions";
    QDir dir(path);
    if (!dir.exists()) {
        return QStringList();
    }

    QStringList result;
    for (const QString &file : dir.entryList(QStringList() << "*.json", QDir::Files)) {
        result.append(file.chopped(5));  // Remove .json
    }
    return result;
}

void Session::saveSession(const QString &name, MainWindow *window)
{
    QJsonObject json = windowToJson(window);

    QFile file(namedSessionPath(name));
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    QJsonDocument doc(json);
    file.write(doc.toJson());
}

void Session::loadSession(const QString &name, MainWindow *window)
{
    QFile file(namedSessionPath(name));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) {
        return;
    }

    jsonToWindow(doc.object(), window);
}

void Session::deleteSession(const QString &name)
{
    QFile::remove(namedSessionPath(name));
}

QString Session::sessionFilePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/session.json";
}

QString Session::namedSessionPath(const QString &name) const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sessions";
    QDir().mkpath(path);
    return path + "/" + name + ".json";
}

QJsonObject Session::windowToJson(MainWindow *window) const
{
    QJsonObject json;

    // Window geometry
    QJsonObject geometry;
    geometry["x"] = window->x();
    geometry["y"] = window->y();
    geometry["width"] = window->width();
    geometry["height"] = window->height();
    geometry["maximized"] = window->isMaximized();
    json["geometry"] = geometry;

    // Open files
    QJsonArray files;
    TabWidget *tabWidget = window->tabWidget();
    for (int i = 0; i < tabWidget->count(); ++i) {
        Editor *editor = qobject_cast<Editor*>(tabWidget->widget(i));
        if (!editor || !editor->document() || editor->document()->isUntitled()) {
            continue;
        }

        QJsonObject docObj;
        docObj["path"] = editor->document()->filePath();

        QJsonArray bookmarksArray;
        for (int bm : editor->bookmarks()) {
            bookmarksArray.append(bm);
        }
        docObj["bookmarks"] = bookmarksArray;

        QJsonArray foldsArray;
        for (int foldLine : editor->foldedRegions().keys()) {
            foldsArray.append(foldLine);
        }
        docObj["foldedRegions"] = foldsArray;

        files.append(docObj);
    }
    json["files"] = files;

    // Active tab
    json["activeTab"] = window->currentTabIndex();

    return json;
}

void Session::jsonToWindow(const QJsonObject &json, MainWindow *window)
{
    // Restore geometry
    if (json.contains("geometry")) {
        QJsonObject geometry = json["geometry"].toObject();
        window->move(geometry["x"].toInt(), geometry["y"].toInt());
        window->resize(geometry["width"].toInt(), geometry["height"].toInt());
        if (geometry["maximized"].toBool()) {
            window->showMaximized();
        }
    }

    // Restore files
    if (json.contains("files")) {
        QJsonArray files = json["files"].toArray();
        for (const QJsonValue &value : files) {
            QJsonObject docObj = value.toObject();
            QString path = docObj["path"].toString();
            if (path.isEmpty()) {
                continue;
            }
            window->openFile(path);

            // Get the editor that was just opened (it will be the current tab)
            Editor *editor = window->currentEditor();
            if (!editor) {
                continue;
            }

            // Restore bookmarks
            QJsonArray bookmarksArray = docObj["bookmarks"].toArray();
            for (const QJsonValue &v : bookmarksArray) {
                editor->toggleBookmark(v.toInt());
            }

            // Restore fold state
            QJsonArray foldsArray = docObj["foldedRegions"].toArray();
            for (const QJsonValue &v : foldsArray) {
                editor->foldAt(v.toInt());
            }
        }
    }

    // Restore active tab
    if (json.contains("activeTab")) {
        window->setCurrentTabIndex(json["activeTab"].toInt());
    }
}

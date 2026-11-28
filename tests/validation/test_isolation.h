#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QString>

#include <cstdio>
#include <cstdlib>

// The suites drive MainWindow, Session and Settings, which read and delete
// files under QStandardPaths. Without isolation they operate on the installed
// editor's real unsaved-document cache, session and settings and destroy them.
// Call after constructing QApplication and before touching any app code.
inline void isolateFromUserData()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("PrimeEditTests"));
    QCoreApplication::setApplicationName(QStringLiteral("PrimeEditTests"));

    const QString realData = QDir::homePath() + QStringLiteral("/.local/share/PrimeEdit");
    const QString realConfig = QDir::homePath() + QStringLiteral("/.config/PrimeEdit");
    const QString data = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString config = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (data.startsWith(realData) || config.startsWith(realConfig)) {
        std::fprintf(stderr, "Refusing to run: test data path resolves to the real PrimeEdit profile (%s)\n",
                     qPrintable(data));
        std::exit(2);
    }
}

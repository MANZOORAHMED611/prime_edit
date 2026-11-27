#pragma once

// Shared fixture for the Phase 1 regression suites (tests/regression).
// Every suite uses PRIMEEDIT_REGRESSION_MAIN so isolateFromUserData() runs
// after QApplication exists and before any app code can touch the profile.

#include "test_isolation.h"

#include <QApplication>
#include <QByteArray>
#include <QFile>
#include <QString>
#include <QTemporaryDir>
#include <QtTest>

namespace regression {

// A scratch directory that is removed when the fixture goes out of scope.
// Suites create every fixture file in one of these, never in the source tree.
class TempDir
{
public:
    TempDir() : m_dir(QDir::tempPath() + QStringLiteral("/primeedit-regression-XXXXXX")) {}

    bool isValid() const { return m_dir.isValid(); }
    QString path() const { return m_dir.path(); }
    QString filePath(const QString &name) const { return m_dir.filePath(name); }

private:
    QTemporaryDir m_dir;
};

// Writes bytes exactly as given (no text mode, no newline translation).
inline bool writeBytes(const QString &path, const QByteArray &bytes)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    return file.write(bytes) == bytes.size() && file.flush();
}

// Reads a file's exact bytes. Returns a null QByteArray if it cannot be opened,
// so a missing file never compares equal to an empty one.
inline QByteArray readBytes(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QByteArray();
    QByteArray bytes = file.readAll();
    if (bytes.isNull())
        bytes = QByteArray("");
    return bytes;
}

// Exit code ctest reports as "Skipped" (SKIP_RETURN_CODE in CMakeLists.txt).
// Only a stub returns it, and only when nothing failed. Every suite exits 1 on
// any failure, so a failing test can never be reported as a skip.
constexpr int kStubSkipExitCode = 77;

} // namespace regression

#define PRIMEEDIT_REGRESSION_MAIN_IMPL(TestClass, passCode)            \
    int main(int argc, char *argv[])                                   \
    {                                                                  \
        QApplication app(argc, argv);                                  \
        isolateFromUserData();                                         \
        TestClass tc;                                                  \
        return QTest::qExec(&tc, argc, argv) == 0 ? (passCode) : 1;    \
    }

// Real suites. Exit 0 when every test passed, 1 otherwise.
#define PRIMEEDIT_REGRESSION_MAIN(TestClass) PRIMEEDIT_REGRESSION_MAIN_IMPL(TestClass, 0)

// T0.2 stubs only. The task that owns a stub replaces it with real tests and
// PRIMEEDIT_REGRESSION_MAIN.
#define PRIMEEDIT_REGRESSION_STUB_MAIN(TestClass) \
    PRIMEEDIT_REGRESSION_MAIN_IMPL(TestClass, regression::kStubSkipExitCode)

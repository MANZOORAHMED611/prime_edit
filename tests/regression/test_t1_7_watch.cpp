// T1.7 regression tests (spec 2026-11-26-phase1-data-loss-fixes, D8 core):
// the editor's own save is not reported as an external modification; a real
// external write still is.
#include "support.h"

#include "core/document.h"
#include "core/documentmanager.h"

#include <QCryptographicHash>
#include <QSignalSpy>

namespace {

// Long enough for inotify events from a save to arrive and be handled.
constexpr int kSettleMs = 700;

bool reportedFor(const QSignalSpy &spy, const QString &path)
{
    for (const QList<QVariant> &args : spy) {
        if (args.value(0).toString() == path)
            return true;
    }
    return false;
}

} // namespace

class TestT17Watch : public QObject
{
    Q_OBJECT

private slots:
    void savedSignalCarriesWrittenBytes();
    void ownSaveEmitsNoExternalModification();
    void externalWriteAfterOwnSaveIsReported();
    void saveAsWatchesTheNewPath();
};

void TestT17Watch::savedSignalCarriesWrittenBytes()
{
    regression::TempDir dir;
    const QString path = dir.filePath("signal.txt");
    QVERIFY(regression::writeBytes(path, "old\n"));
    Document doc;
    QVERIFY(doc.load(path));
    QSignalSpy saved(&doc, &Document::saved);
    doc.setText("new text\n");

    QCOMPARE(doc.save(), SaveResult::Ok);
    QCOMPARE(saved.count(), 1);
    const QByteArray bytes("new text\n");
    QCOMPARE(saved.first().at(0).toString(), path);
    QCOMPARE(saved.first().at(1).toByteArray(),
             QCryptographicHash::hash(bytes, QCryptographicHash::Sha1));
    QCOMPARE(saved.first().at(2).toLongLong(), qint64(bytes.size()));

    doc.setReadOnly(true);
    QCOMPARE(doc.save(), SaveResult::ReadOnly);
    QCOMPARE(saved.count(), 1);
}

void TestT17Watch::ownSaveEmitsNoExternalModification()
{
    regression::TempDir dir;
    const QString path = dir.filePath("own.txt");
    QVERIFY(regression::writeBytes(path, "first\n"));
    DocumentManager &manager = DocumentManager::instance();
    Document *doc = manager.openDocument(path);
    QVERIFY(doc);
    QSignalSpy spy(&manager, &DocumentManager::fileExternallyModified);

    for (int i = 0; i < 3; ++i) {
        doc->setText(QString("edit %1\n").arg(i));
        QCOMPARE(doc->save(), SaveResult::Ok);
        QTest::qWait(kSettleMs);
    }
    QVERIFY2(!reportedFor(spy, path), "own save reported as an external change");
    manager.closeDocument(doc, true);
}

void TestT17Watch::externalWriteAfterOwnSaveIsReported()
{
    regression::TempDir dir;
    const QString path = dir.filePath("external.txt");
    QVERIFY(regression::writeBytes(path, "first\n"));
    DocumentManager &manager = DocumentManager::instance();
    Document *doc = manager.openDocument(path);
    QVERIFY(doc);
    QSignalSpy spy(&manager, &DocumentManager::fileExternallyModified);

    doc->setText("mine\n");
    QCOMPARE(doc->save(), SaveResult::Ok);
    QTest::qWait(kSettleMs);
    QVERIFY2(!reportedFor(spy, path), "own save reported as an external change");

    QVERIFY(regression::writeBytes(path, "someone else wrote this\n"));
    QTRY_VERIFY_WITH_TIMEOUT(reportedFor(spy, path), 5000);
    manager.closeDocument(doc, true);
}

void TestT17Watch::saveAsWatchesTheNewPath()
{
    regression::TempDir dir;
    const QString path = dir.filePath("saved-as.txt");
    DocumentManager &manager = DocumentManager::instance();
    Document *doc = manager.createDocument();
    QSignalSpy spy(&manager, &DocumentManager::fileExternallyModified);

    doc->setText("untitled text\n");
    QCOMPARE(doc->saveAs(path), SaveResult::Ok);
    QTest::qWait(kSettleMs);
    QVERIFY(!reportedFor(spy, path));

    QVERIFY(regression::writeBytes(path, "changed outside\n"));
    QTRY_VERIFY_WITH_TIMEOUT(reportedFor(spy, path), 5000);
    manager.closeDocument(doc, true);
}

PRIMEEDIT_REGRESSION_MAIN(TestT17Watch)

#include "test_t1_7_watch.moc"

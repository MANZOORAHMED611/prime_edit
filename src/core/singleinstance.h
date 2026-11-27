#ifndef SINGLEINSTANCE_H
#define SINGLEINSTANCE_H

#include <QList>
#include <QObject>
#include <QStringList>
#include <memory>

class QLocalServer;
class QLocalSocket;
class QLockFile;

// One writer per profile (spec 3 D4). The first launch holds a QLockFile at
// AppDataLocation/instance.lock for its lifetime and listens on a per-profile
// local socket; a later launch forwards its files there and exits.
//
// Protocol: the client sends one line {"v":1,"files":[absolute paths],
// "goto":<int or null>} and the holder answers {"ack":true} after queueing the
// opens. Lives in prime-edit-core so the suites can link it.
class SingleInstance : public QObject
{
    Q_OBJECT

public:
    explicit SingleInstance(QObject *parent = nullptr);
    ~SingleInstance() override;

    // $XDG_RUNTIME_DIR/prime-edit-<first 16 hex of SHA-1(AppDataLocation)>.sock,
    // or AppDataLocation/instance.sock without XDG_RUNTIME_DIR.
    static QString serverName();
    static QString lockPath();

    // Takes the profile lock without waiting. False means another instance
    // holds it (a crashed holder's lock is recovered by QLockFile).
    bool acquire();
    // Removes a stale socket left by a crashed holder, then listens.
    bool listen();

    // Client side, before any event loop: blocks at most connectMs to connect
    // and ackMs for the ack. gotoLine < 1 is sent as null. True on ack.
    static bool forward(const QStringList &absoluteFiles, int gotoLine,
                        int connectMs = 2000, int ackMs = 5000);

signals:
    // Emitted once per forwarded launch; gotoLine is 0 when none was given.
    void openRequested(const QStringList &files, int gotoLine);

protected:
    void connectNotify(const QMetaMethod &signal) override;

private:
    // A forward acked before anything is connected to openRequested (startup
    // is still restoring, and a dialog may run the event loop) is held here and
    // delivered once a receiver connects, so an acked open is never dropped.
    struct PendingOpen {
        QStringList files;
        int gotoLine = 0;
    };
    QList<PendingOpen> m_pending;

    void deliver(const QStringList &files, int gotoLine);
    void flushPending();
    void onNewConnection();
    void handleRequest(QLocalSocket *socket);

    std::unique_ptr<QLockFile> m_lock;
    QLocalServer *m_server = nullptr;
};

#endif // SINGLEINSTANCE_H

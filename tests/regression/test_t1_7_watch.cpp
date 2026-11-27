// Stub for T1.7 (spec section 9.2). The task named here replaces this file
// with its regression tests; section 9.5 requires no "pending" stub to survive.
#include "support.h"

class TestT17Watch : public QObject
{
    Q_OBJECT

private slots:
    void pending() { QSKIP("pending T1.7"); }
};

PRIMEEDIT_REGRESSION_STUB_MAIN(TestT17Watch)

#include "test_t1_7_watch.moc"

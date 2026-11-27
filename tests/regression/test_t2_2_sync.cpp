// Stub for T2.2 (spec section 9.2). The task named here replaces this file
// with its regression tests; section 9.5 requires no "pending" stub to survive.
#include "support.h"

class TestT22Sync : public QObject
{
    Q_OBJECT

private slots:
    void pending() { QSKIP("pending T2.2"); }
};

PRIMEEDIT_REGRESSION_STUB_MAIN(TestT22Sync)

#include "test_t2_2_sync.moc"

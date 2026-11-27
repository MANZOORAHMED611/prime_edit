// Stub for T1.5 (spec section 9.2). The task named here replaces this file
// with its regression tests; section 9.5 requires no "pending" stub to survive.
#include "support.h"

class TestT15Cache : public QObject
{
    Q_OBJECT

private slots:
    void pending() { QSKIP("pending T1.5"); }
};

PRIMEEDIT_REGRESSION_STUB_MAIN(TestT15Cache)

#include "test_t1_5_cache.moc"

// Stub for T1.4 (spec section 9.2). The task named here replaces this file
// with its regression tests; section 9.5 requires no "pending" stub to survive.
#include "support.h"

class TestT14Persistence : public QObject
{
    Q_OBJECT

private slots:
    void pending() { QSKIP("pending T1.4"); }
};

PRIMEEDIT_REGRESSION_STUB_MAIN(TestT14Persistence)

#include "test_t1_4_persistence.moc"

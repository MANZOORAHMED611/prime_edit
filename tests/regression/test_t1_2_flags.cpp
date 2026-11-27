// Stub for T1.2 (spec section 9.2). The task named here replaces this file
// with its regression tests; section 9.5 requires no "pending" stub to survive.
#include "support.h"

class TestT12Flags : public QObject
{
    Q_OBJECT

private slots:
    void pending() { QSKIP("pending T1.2"); }
};

PRIMEEDIT_REGRESSION_STUB_MAIN(TestT12Flags)

#include "test_t1_2_flags.moc"

// Stub for T4.1 (spec section 9.2). The task named here replaces this file
// with its regression tests; section 9.5 requires no "pending" stub to survive.
#include "support.h"

class TestI1Roundtrip : public QObject
{
    Q_OBJECT

private slots:
    void pending() { QSKIP("pending T4.1"); }
};

PRIMEEDIT_REGRESSION_STUB_MAIN(TestI1Roundtrip)

#include "test_i1_roundtrip.moc"

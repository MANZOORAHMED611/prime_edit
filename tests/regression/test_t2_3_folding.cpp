// Stub for T2.3 (spec section 9.2). The task named here replaces this file
// with its regression tests; section 9.5 requires no "pending" stub to survive.
#include "support.h"

class TestT23Folding : public QObject
{
    Q_OBJECT

private slots:
    void pending() { QSKIP("pending T2.3"); }
};

PRIMEEDIT_REGRESSION_STUB_MAIN(TestT23Folding)

#include "test_t2_3_folding.moc"

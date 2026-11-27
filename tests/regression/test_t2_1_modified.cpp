// Stub for T2.1 (spec section 9.2). The task named here replaces this file
// with its regression tests; section 9.5 requires no "pending" stub to survive.
#include "support.h"

class TestT21Modified : public QObject
{
    Q_OBJECT

private slots:
    void pending() { QSKIP("pending T2.1"); }
};

PRIMEEDIT_REGRESSION_STUB_MAIN(TestT21Modified)

#include "test_t2_1_modified.moc"

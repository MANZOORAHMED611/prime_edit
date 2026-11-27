// Stub for T1.1 (spec section 9.2). The task named here replaces this file
// with its regression tests; section 9.5 requires no "pending" stub to survive.
#include "support.h"

class TestT11Save : public QObject
{
    Q_OBJECT

private slots:
    void pending() { QSKIP("pending T1.1"); }
};

PRIMEEDIT_REGRESSION_STUB_MAIN(TestT11Save)

#include "test_t1_1_save.moc"

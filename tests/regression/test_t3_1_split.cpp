// Stub for T3.1 (spec section 9.2). The task named here replaces this file
// with its regression tests; section 9.5 requires no "pending" stub to survive.
#include "support.h"

class TestT31Split : public QObject
{
    Q_OBJECT

private slots:
    void pending() { QSKIP("pending T3.1"); }
};

PRIMEEDIT_REGRESSION_STUB_MAIN(TestT31Split)

#include "test_t3_1_split.moc"

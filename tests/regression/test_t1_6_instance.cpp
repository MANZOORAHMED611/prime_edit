// Stub for T1.6 (spec section 9.2). The task named here replaces this file
// with its regression tests; section 9.5 requires no "pending" stub to survive.
#include "support.h"

class TestT16Instance : public QObject
{
    Q_OBJECT

private slots:
    void pending() { QSKIP("pending T1.6"); }
};

PRIMEEDIT_REGRESSION_STUB_MAIN(TestT16Instance)

#include "test_t1_6_instance.moc"

// Stub for T2.4 (spec section 9.2). The task named here replaces this file
// with its regression tests; section 9.5 requires no "pending" stub to survive.
#include "support.h"

class TestT24Readonly : public QObject
{
    Q_OBJECT

private slots:
    void pending() { QSKIP("pending T2.4"); }
};

PRIMEEDIT_REGRESSION_STUB_MAIN(TestT24Readonly)

#include "test_t2_4_readonly.moc"

// Stub for T3.4 (spec section 9.2). The task named here replaces this file
// with its regression tests; section 9.5 requires no "pending" stub to survive.
#include "support.h"

class TestT34Reload : public QObject
{
    Q_OBJECT

private slots:
    void pending() { QSKIP("pending T3.4"); }
};

PRIMEEDIT_REGRESSION_STUB_MAIN(TestT34Reload)

#include "test_t3_4_reload.moc"

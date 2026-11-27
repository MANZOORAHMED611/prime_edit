// Stub for T3.2 (spec section 9.2). The task named here replaces this file
// with its regression tests; section 9.5 requires no "pending" stub to survive.
#include "support.h"

class TestT32SaveUi : public QObject
{
    Q_OBJECT

private slots:
    void pending() { QSKIP("pending T3.2"); }
};

PRIMEEDIT_REGRESSION_STUB_MAIN(TestT32SaveUi)

#include "test_t3_2_save_ui.moc"

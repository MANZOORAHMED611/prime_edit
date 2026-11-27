// Stub for T3.3 (spec section 9.2). The task named here replaces this file
// with its regression tests; section 9.5 requires no "pending" stub to survive.
#include "support.h"

class TestT33EncodingUi : public QObject
{
    Q_OBJECT

private slots:
    void pending() { QSKIP("pending T3.3"); }
};

PRIMEEDIT_REGRESSION_STUB_MAIN(TestT33EncodingUi)

#include "test_t3_3_encoding_ui.moc"

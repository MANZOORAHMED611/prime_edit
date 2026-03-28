#include "macrodialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

MacroDialog::MacroDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Run Macro Multiple Times"));
    QVBoxLayout *layout = new QVBoxLayout(this);

    m_runNTimes = new QRadioButton(tr("Run"), this);
    m_runNTimes->setChecked(true);
    m_countSpin = new QSpinBox(this);
    m_countSpin->setRange(1, 999999);
    m_countSpin->setValue(10);
    QLabel *timesLabel = new QLabel(tr("times"), this);

    QHBoxLayout *nLayout = new QHBoxLayout;
    nLayout->addWidget(m_runNTimes);
    nLayout->addWidget(m_countSpin);
    nLayout->addWidget(timesLabel);
    nLayout->addStretch();
    layout->addLayout(nLayout);

    m_runUntilEnd = new QRadioButton(tr("Run until end of file"), this);
    layout->addWidget(m_runUntilEnd);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

int MacroDialog::runCount() const { return m_countSpin->value(); }
bool MacroDialog::runUntilEnd() const { return m_runUntilEnd->isChecked(); }

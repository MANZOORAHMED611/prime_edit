#include "columneditor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>

ColumnEditor::ColumnEditor(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Column Editor"));
    setupUi();
}

void ColumnEditor::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Text mode
    QGroupBox *textGroup = new QGroupBox(tr("Text to Insert"));
    QHBoxLayout *textLayout = new QHBoxLayout(textGroup);
    m_textRadio = new QRadioButton(tr("Text:"));
    m_textRadio->setChecked(true);
    m_textEdit = new QLineEdit;
    textLayout->addWidget(m_textRadio);
    textLayout->addWidget(m_textEdit);
    mainLayout->addWidget(textGroup);

    // Number mode
    QGroupBox *numGroup = new QGroupBox(tr("Number to Insert"));
    QVBoxLayout *numLayout = new QVBoxLayout(numGroup);
    m_numberRadio = new QRadioButton(tr("Number:"));

    QHBoxLayout *initLayout = new QHBoxLayout;
    initLayout->addWidget(new QLabel(tr("Initial number:")));
    m_initialSpin = new QSpinBox;
    m_initialSpin->setRange(0, 999999);
    m_initialSpin->setValue(1);
    initLayout->addWidget(m_initialSpin);

    QHBoxLayout *incLayout = new QHBoxLayout;
    incLayout->addWidget(new QLabel(tr("Increase by:")));
    m_increaseSpin = new QSpinBox;
    m_increaseSpin->setRange(1, 1000);
    m_increaseSpin->setValue(1);
    incLayout->addWidget(m_increaseSpin);

    m_leadingZerosCheck = new QCheckBox(tr("Leading zeros"));

    numLayout->addWidget(m_numberRadio);
    numLayout->addLayout(initLayout);
    numLayout->addLayout(incLayout);
    numLayout->addWidget(m_leadingZerosCheck);
    mainLayout->addWidget(numGroup);

    // Buttons
    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

ColumnEditor::Mode ColumnEditor::mode() const
{
    return m_numberRadio->isChecked() ? NumberMode : TextMode;
}

QString ColumnEditor::text() const
{
    return m_textEdit->text();
}

int ColumnEditor::initialNumber() const
{
    return m_initialSpin->value();
}

int ColumnEditor::increaseBy() const
{
    return m_increaseSpin->value();
}

bool ColumnEditor::leadingZeros() const
{
    return m_leadingZerosCheck->isChecked();
}

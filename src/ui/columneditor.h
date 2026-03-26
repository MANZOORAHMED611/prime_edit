#ifndef COLUMNEDITOR_H
#define COLUMNEDITOR_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QRadioButton>

class ColumnEditor : public QDialog
{
    Q_OBJECT
public:
    explicit ColumnEditor(QWidget *parent = nullptr);

    enum Mode { TextMode, NumberMode };

    Mode mode() const;
    QString text() const;
    int initialNumber() const;
    int increaseBy() const;
    bool leadingZeros() const;

private:
    void setupUi();

    QRadioButton *m_textRadio;
    QRadioButton *m_numberRadio;
    QLineEdit *m_textEdit;
    QSpinBox *m_initialSpin;
    QSpinBox *m_increaseSpin;
    QCheckBox *m_leadingZerosCheck;
};

#endif // COLUMNEDITOR_H

#ifndef MACRODIALOG_H
#define MACRODIALOG_H
#include <QDialog>
#include <QRadioButton>
#include <QSpinBox>

class MacroDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MacroDialog(QWidget *parent = nullptr);
    int runCount() const;
    bool runUntilEnd() const;
private:
    QRadioButton *m_runNTimes;
    QRadioButton *m_runUntilEnd;
    QSpinBox *m_countSpin;
};
#endif

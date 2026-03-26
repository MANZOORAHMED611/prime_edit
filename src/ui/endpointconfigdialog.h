#ifndef ENDPOINTCONFIGDIALOG_H
#define ENDPOINTCONFIGDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>
#include "core/llmevaluator.h"

class EndpointConfigDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EndpointConfigDialog(QWidget *parent = nullptr);

    EvalEndpoint endpoint() const;
    void setEndpoint(const EvalEndpoint &endpoint);

private:
    QLineEdit *m_nameEdit;
    QLineEdit *m_urlEdit;
    QLineEdit *m_modelEdit;
    QLineEdit *m_apiKeyEdit;
    QTextEdit *m_systemPromptEdit;
    QComboBox *m_typeCombo;
};

#endif // ENDPOINTCONFIGDIALOG_H

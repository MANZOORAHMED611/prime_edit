#include "endpointconfigdialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>

EndpointConfigDialog::EndpointConfigDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Configure LLM Endpoint"));
    setMinimumWidth(450);

    auto *mainLayout = new QVBoxLayout(this);

    auto *formLayout = new QFormLayout;

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("Profile name"));
    formLayout->addRow(tr("&Name:"), m_nameEdit);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem(tr("Ollama"), static_cast<int>(EvalEndpoint::Ollama));
    m_typeCombo->addItem(tr("OpenAI Compatible"), static_cast<int>(EvalEndpoint::OpenAICompatible));
    formLayout->addRow(tr("&Type:"), m_typeCombo);

    m_urlEdit = new QLineEdit(this);
    m_urlEdit->setPlaceholderText(tr("http://localhost:11434/api/generate"));
    formLayout->addRow(tr("&URL:"), m_urlEdit);

    m_modelEdit = new QLineEdit(this);
    m_modelEdit->setPlaceholderText(tr("llama3"));
    formLayout->addRow(tr("&Model:"), m_modelEdit);

    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText(tr("Optional for Ollama"));
    formLayout->addRow(tr("API &Key:"), m_apiKeyEdit);

    m_systemPromptEdit = new QTextEdit(this);
    m_systemPromptEdit->setMaximumHeight(100);
    m_systemPromptEdit->setPlaceholderText(tr("System prompt for the model"));
    formLayout->addRow(tr("System &Prompt:"), m_systemPromptEdit);

    mainLayout->addLayout(formLayout);

    // Update URL placeholder when type changes
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        auto type = static_cast<EvalEndpoint::Type>(m_typeCombo->itemData(index).toInt());
        if (type == EvalEndpoint::Ollama) {
            m_urlEdit->setPlaceholderText("http://localhost:11434/api/generate");
        } else {
            m_urlEdit->setPlaceholderText("https://api.openai.com/v1/chat/completions");
        }
    });

    auto *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

EvalEndpoint EndpointConfigDialog::endpoint() const
{
    EvalEndpoint ep;
    ep.name = m_nameEdit->text();
    ep.url = m_urlEdit->text();
    ep.model = m_modelEdit->text();
    ep.apiKey = m_apiKeyEdit->text();
    ep.systemPrompt = m_systemPromptEdit->toPlainText();
    ep.type = static_cast<EvalEndpoint::Type>(
        m_typeCombo->currentData().toInt());
    return ep;
}

void EndpointConfigDialog::setEndpoint(const EvalEndpoint &endpoint)
{
    m_nameEdit->setText(endpoint.name);
    m_urlEdit->setText(endpoint.url);
    m_modelEdit->setText(endpoint.model);
    m_apiKeyEdit->setText(endpoint.apiKey);
    m_systemPromptEdit->setPlainText(endpoint.systemPrompt);

    int typeIndex = m_typeCombo->findData(static_cast<int>(endpoint.type));
    if (typeIndex >= 0) {
        m_typeCombo->setCurrentIndex(typeIndex);
    }
}

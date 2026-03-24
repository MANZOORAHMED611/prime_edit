#include "statusbar.h"
#include "editor.h"
#include "core/document.h"

StatusBarWidget::StatusBarWidget(QWidget *parent)
    : QStatusBar(parent)
{
    m_infoLabel = new QLabel(this);
    m_positionLabel = new QLabel(this);
    m_languageLabel = new QLabel(this);
    m_encodingLabel = new QLabel(this);
    m_lineEndingLabel = new QLabel(this);

    // Add widgets to status bar
    addWidget(m_infoLabel, 1);  // Stretch
    addPermanentWidget(m_positionLabel);
    addPermanentWidget(m_languageLabel);
    addPermanentWidget(m_encodingLabel);
    addPermanentWidget(m_lineEndingLabel);

    // Set default values
    setPosition(1, 1);
    setEncoding("UTF-8");
    setLineEnding("LF");
    setLanguage("Plain Text");

    // Style
    setStyleSheet("QStatusBar { border-top: 1px solid #3a3a3a; } "
                  "QLabel { padding: 2px 8px; }");
}

void StatusBarWidget::updateFromEditor(Editor *editor)
{
    if (!editor) {
        return;
    }

    setPosition(editor->currentLine(), editor->currentColumn());

    Document *doc = editor->document();
    if (doc) {
        setEncoding(doc->encoding());

        QString lineEnding;
        switch (doc->lineEnding()) {
        case Document::Unix: lineEnding = "LF"; break;
        case Document::Windows: lineEnding = "CRLF"; break;
        case Document::ClassicMac: lineEnding = "CR"; break;
        }
        setLineEnding(lineEnding);

        QString lang = doc->language();
        setLanguage(lang.isEmpty() ? "Plain Text" : lang);
    }
}

void StatusBarWidget::setPosition(int line, int column)
{
    m_positionLabel->setText(tr("Ln %1, Col %2").arg(line).arg(column));
}

void StatusBarWidget::setEncoding(const QString &encoding)
{
    m_encodingLabel->setText(encoding);
}

void StatusBarWidget::setLineEnding(const QString &lineEnding)
{
    m_lineEndingLabel->setText(lineEnding);
}

void StatusBarWidget::setLanguage(const QString &language)
{
    m_languageLabel->setText(language);
}

void StatusBarWidget::setFileInfo(const QString &info)
{
    m_infoLabel->setText(info);
}

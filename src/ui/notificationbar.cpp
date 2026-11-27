#include "notificationbar.h"
#include <QHBoxLayout>

NotificationBar::NotificationBar(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(
        "background-color: #FFF3CD; "
        "border: 1px solid #FFE69C; "
        "padding: 4px;");

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);

    m_messageLabel = new QLabel(this);
    m_yesButton = new QPushButton(tr("Yes"), this);
    m_noButton = new QPushButton(tr("No"), this);
    m_dismissButton = new QPushButton(tr("Dismiss"), this);

    layout->addWidget(m_messageLabel, 1);
    layout->addWidget(m_yesButton);
    layout->addWidget(m_noButton);
    layout->addWidget(m_dismissButton);

    connect(m_yesButton, &QPushButton::clicked, this, [this]() {
        emit accepted();
        hide();
    });
    connect(m_noButton, &QPushButton::clicked, this, [this]() {
        emit rejected();
        hide();
    });
    // Dismissing an informational message answers no question, so it emits
    // neither accepted() nor rejected(): a pending reload must not be triggered.
    connect(m_dismissButton, &QPushButton::clicked, this, &QWidget::hide);

    setQuestionMode(true);
    hide();
}

void NotificationBar::showMessage(const QString &message)
{
    setQuestionMode(true);
    m_messageLabel->setText(message);
    show();
}

void NotificationBar::showInfo(const QString &message)
{
    setQuestionMode(false);
    m_messageLabel->setText(message);
    show();
}

void NotificationBar::setQuestionMode(bool question)
{
    m_yesButton->setVisible(question);
    m_noButton->setVisible(question);
    m_dismissButton->setVisible(!question);
}

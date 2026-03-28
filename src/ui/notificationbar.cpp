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

    layout->addWidget(m_messageLabel, 1);
    layout->addWidget(m_yesButton);
    layout->addWidget(m_noButton);

    connect(m_yesButton, &QPushButton::clicked, this, [this]() {
        emit accepted();
        hide();
    });
    connect(m_noButton, &QPushButton::clicked, this, [this]() {
        emit rejected();
        hide();
    });

    hide();
}

void NotificationBar::showMessage(const QString &message)
{
    m_messageLabel->setText(message);
    show();
}

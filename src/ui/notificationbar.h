#ifndef NOTIFICATIONBAR_H
#define NOTIFICATIONBAR_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

class NotificationBar : public QWidget
{
    Q_OBJECT

public:
    explicit NotificationBar(QWidget *parent = nullptr);

    void showMessage(const QString &message);
    void showInfo(const QString &message);
    // A message with one action button. Clicking it emits actionTriggered(),
    // never accepted(), so it cannot answer a pending reload question.
    void showAction(const QString &message, const QString &actionLabel);
    // A question with custom labels: acceptLabel emits accepted(),
    // rejectLabel emits rejected().
    void showChoice(const QString &message, const QString &acceptLabel, const QString &rejectLabel);

signals:
    void accepted();
    void rejected();
    void actionTriggered();

private:
    void setQuestionMode(bool question);
    void setActionMode();

    QLabel *m_messageLabel;
    QPushButton *m_yesButton;
    QPushButton *m_noButton;
    QPushButton *m_dismissButton;
    QPushButton *m_actionButton;
};

#endif // NOTIFICATIONBAR_H

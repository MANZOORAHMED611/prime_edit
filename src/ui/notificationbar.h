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

signals:
    void accepted();
    void rejected();

private:
    void setQuestionMode(bool question);

    QLabel *m_messageLabel;
    QPushButton *m_yesButton;
    QPushButton *m_noButton;
    QPushButton *m_dismissButton;
};

#endif // NOTIFICATIONBAR_H

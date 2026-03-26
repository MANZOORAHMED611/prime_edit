#ifndef RECOVERYDIALOG_H
#define RECOVERYDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QStringList>

class RecoveryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RecoveryDialog(const QStringList &files,
                            QWidget *parent = nullptr);

    QStringList selectedFiles() const;
    bool shouldRecover() const { return m_recover; }

private:
    QListWidget *m_listWidget;
    bool m_recover = false;
};

#endif // RECOVERYDIALOG_H

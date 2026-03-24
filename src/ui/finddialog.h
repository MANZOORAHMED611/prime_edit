#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTextDocument>

class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(QWidget *parent = nullptr);

    QString searchText() const;
    QString replaceText() const;
    QTextDocument::FindFlags searchFlags() const;

    void setReplaceMode(bool replace);
    bool isReplaceMode() const { return m_replaceMode; }

    void setSearchText(const QString &text);

signals:
    void findNext();
    void findPrevious();
    void replaceOne();
    void replaceAll();

protected:
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onFindNext();
    void onFindPrevious();
    void onReplace();
    void onReplaceAll();

private:
    void setupUi();
    void updateReplaceVisibility();

    QLineEdit *m_searchEdit;
    QLineEdit *m_replaceEdit;
    QLabel *m_replaceLabel;

    QCheckBox *m_caseSensitiveCheck;
    QCheckBox *m_wholeWordCheck;
    QCheckBox *m_regexCheck;
    QCheckBox *m_wrapAroundCheck;

    QPushButton *m_findNextButton;
    QPushButton *m_findPrevButton;
    QPushButton *m_replaceButton;
    QPushButton *m_replaceAllButton;
    QPushButton *m_closeButton;

    QLabel *m_statusLabel;

    bool m_replaceMode = false;
};

#endif // FINDDIALOG_H

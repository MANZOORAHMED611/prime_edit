#ifndef INCREMENTALSEARCHBAR_H
#define INCREMENTALSEARCHBAR_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class IncrementalSearchBar : public QWidget
{
    Q_OBJECT
public:
    explicit IncrementalSearchBar(QWidget *parent = nullptr);

    void activate();
    void deactivate();
    QString searchText() const;

signals:
    void searchChanged(const QString &text);
    void findNext();
    void findPrevious();
    void closed();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QLineEdit *m_searchEdit;
    QPushButton *m_nextButton;
    QPushButton *m_prevButton;
    QPushButton *m_closeButton;
    QLabel *m_matchCountLabel;
};

#endif

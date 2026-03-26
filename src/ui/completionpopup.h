#ifndef COMPLETIONPOPUP_H
#define COMPLETIONPOPUP_H

#include <QListWidget>
#include <QVector>
#include "../core/lspclient.h"

struct SimpleCompletionItem {
    QString label;
    QString detail;
    enum Kind { Keyword, Type, Word, Snippet } kind = Word;
};

class CompletionPopup : public QListWidget
{
    Q_OBJECT

public:
    explicit CompletionPopup(QWidget *parent = nullptr);

    void setCompletions(const QVector<CompletionItem> &items);
    void setSimpleCompletions(const QVector<SimpleCompletionItem> &items);
    void showAtPosition(const QPoint &globalPos);
    QString selectedCompletion() const;

signals:
    void completionSelected(const QString &completion);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private slots:
    void onItemActivated(QListWidgetItem *item);

private:
    QVector<CompletionItem> m_items;
};

#endif // COMPLETIONPOPUP_H

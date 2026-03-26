#ifndef EVALRESULTWIDGET_H
#define EVALRESULTWIDGET_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>

class EvalResultWidget : public QWidget
{
    Q_OBJECT
public:
    explicit EvalResultWidget(QWidget *parent = nullptr);

    void showResult(const QString &original, const QString &result);
    void showLoading();
    QString result() const;

signals:
    void accepted(const QString &result);
    void rejected();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void buildDiffView(const QString &original, const QString &result);

    QPlainTextEdit *m_resultView;
    QPushButton *m_acceptButton;
    QPushButton *m_rejectButton;
    QLabel *m_statusLabel;
    QString m_result;
};

#endif // EVALRESULTWIDGET_H

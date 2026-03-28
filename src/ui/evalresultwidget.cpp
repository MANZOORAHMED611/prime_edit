#include "evalresultwidget.h"
#include "theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QTextCharFormat>
#include <QTextCursor>

EvalResultWidget::EvalResultWidget(QWidget *parent) : QWidget(parent)
{
    Theme theme = ThemeManager::instance().currentTheme();
    setStyleSheet(QString("background-color: %1; border: 2px solid %2; border-radius: 4px;")
        .arg(theme.menuBackground.name(), theme.accentPrimary.name()));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    m_statusLabel = new QLabel(tr("Evaluation Result"), this);
    m_statusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(theme.menuForeground.name()));
    layout->addWidget(m_statusLabel);

    m_resultView = new QPlainTextEdit(this);
    m_resultView->setReadOnly(true);
    m_resultView->setStyleSheet(QString("background-color: %1; color: %2; border: 1px solid %3;")
        .arg(theme.background.name(), theme.foreground.name(), theme.borderColor.name()));
    m_resultView->setMaximumHeight(200);
    layout->addWidget(m_resultView);

    auto *btnLayout = new QHBoxLayout;
    m_acceptButton = new QPushButton(tr("Accept (A)"), this);
    m_acceptButton->setStyleSheet(
        "background-color: #50fa7b; color: #000; padding: 4px 12px;");
    m_rejectButton = new QPushButton(tr("Reject (Esc)"), this);
    m_rejectButton->setStyleSheet(
        "background-color: #ff5555; color: #fff; padding: 4px 12px;");

    btnLayout->addStretch();
    btnLayout->addWidget(m_acceptButton);
    btnLayout->addWidget(m_rejectButton);
    layout->addLayout(btnLayout);

    connect(m_acceptButton, &QPushButton::clicked, this, [this]() {
        emit accepted(m_result);
    });
    connect(m_rejectButton, &QPushButton::clicked, this, [this]() {
        emit rejected();
    });

    hide();
    setFocusPolicy(Qt::StrongFocus);
}

void EvalResultWidget::showResult(const QString &original, const QString &result)
{
    m_result = result;
    m_statusLabel->setText(tr("Evaluation Result \u2014 Press A to accept, Esc to reject"));
    buildDiffView(original, result);
    show();
    setFocus();
}

void EvalResultWidget::showLoading()
{
    m_resultView->setPlainText(tr("Evaluating..."));
    m_statusLabel->setText(tr("Sending to endpoint..."));
    m_acceptButton->setEnabled(false);
    show();
}

QString EvalResultWidget::result() const
{
    return m_result;
}

void EvalResultWidget::buildDiffView(const QString &original, const QString &result)
{
    m_acceptButton->setEnabled(true);

    m_resultView->clear();

    QStringList origLines = original.split('\n');
    QStringList resLines = result.split('\n');

    QTextCursor cursor(m_resultView->document());

    QTextCharFormat addedFormat;
    addedFormat.setBackground(QColor(80, 250, 123, 40));
    addedFormat.setForeground(QColor(248, 248, 242));

    QTextCharFormat removedFormat;
    removedFormat.setBackground(QColor(255, 85, 85, 40));
    removedFormat.setForeground(QColor(248, 248, 242));
    removedFormat.setFontStrikeOut(true);

    QTextCharFormat normalFormat;
    normalFormat.setForeground(QColor(248, 248, 242));

    int maxLines = qMax(origLines.size(), resLines.size());
    for (int i = 0; i < maxLines; ++i) {
        QString origLine = (i < origLines.size()) ? origLines[i] : QString();
        QString resLine = (i < resLines.size()) ? resLines[i] : QString();

        if (origLine == resLine) {
            cursor.insertText("  " + resLine + "\n", normalFormat);
        } else {
            if (!origLine.isEmpty()) {
                cursor.insertText("- " + origLine + "\n", removedFormat);
            }
            if (!resLine.isEmpty()) {
                cursor.insertText("+ " + resLine + "\n", addedFormat);
            }
        }
    }
}

void EvalResultWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_A) {
        emit accepted(m_result);
    } else if (event->key() == Qt::Key_Escape) {
        emit rejected();
    }
    QWidget::keyPressEvent(event);
}

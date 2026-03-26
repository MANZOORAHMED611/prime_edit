#include "incrementalsearchbar.h"
#include <QHBoxLayout>
#include <QKeyEvent>

IncrementalSearchBar::IncrementalSearchBar(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(4);

    QLabel *label = new QLabel(tr("Search:"), this);
    layout->addWidget(label);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setMinimumWidth(200);
    m_searchEdit->setFixedHeight(24);
    m_searchEdit->installEventFilter(this);
    layout->addWidget(m_searchEdit);

    m_prevButton = new QPushButton(tr("<"), this);
    m_prevButton->setFixedSize(28, 24);
    m_prevButton->setToolTip(tr("Find Previous (Shift+Enter)"));
    layout->addWidget(m_prevButton);

    m_nextButton = new QPushButton(tr(">"), this);
    m_nextButton->setFixedSize(28, 24);
    m_nextButton->setToolTip(tr("Find Next (Enter)"));
    layout->addWidget(m_nextButton);

    m_matchCountLabel = new QLabel(this);
    m_matchCountLabel->setMinimumWidth(80);
    layout->addWidget(m_matchCountLabel);

    m_closeButton = new QPushButton(tr("X"), this);
    m_closeButton->setFixedSize(24, 24);
    m_closeButton->setToolTip(tr("Close (Escape)"));
    layout->addWidget(m_closeButton);

    layout->addStretch();

    setFixedHeight(32);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &IncrementalSearchBar::searchChanged);
    connect(m_nextButton, &QPushButton::clicked, this, &IncrementalSearchBar::findNext);
    connect(m_prevButton, &QPushButton::clicked, this, &IncrementalSearchBar::findPrevious);
    connect(m_closeButton, &QPushButton::clicked, this, &IncrementalSearchBar::deactivate);
}

void IncrementalSearchBar::activate()
{
    show();
    m_searchEdit->setFocus();
    m_searchEdit->selectAll();
}

void IncrementalSearchBar::deactivate()
{
    hide();
    emit closed();
}

QString IncrementalSearchBar::searchText() const
{
    return m_searchEdit->text();
}

bool IncrementalSearchBar::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_searchEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            deactivate();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                emit findPrevious();
            } else {
                emit findNext();
            }
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

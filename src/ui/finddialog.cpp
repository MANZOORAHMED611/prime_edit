#include "finddialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QKeyEvent>
#include <QShowEvent>

FindDialog::FindDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
    setWindowTitle(tr("Find"));
}

void FindDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Search input
    QGridLayout *inputLayout = new QGridLayout();

    QLabel *searchLabel = new QLabel(tr("Find:"), this);
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setMinimumWidth(300);

    inputLayout->addWidget(searchLabel, 0, 0);
    inputLayout->addWidget(m_searchEdit, 0, 1);

    m_replaceLabel = new QLabel(tr("Replace:"), this);
    m_replaceEdit = new QLineEdit(this);

    inputLayout->addWidget(m_replaceLabel, 1, 0);
    inputLayout->addWidget(m_replaceEdit, 1, 1);

    mainLayout->addLayout(inputLayout);

    // Options
    QHBoxLayout *optionsLayout = new QHBoxLayout();

    m_caseSensitiveCheck = new QCheckBox(tr("Case sensitive"), this);
    m_wholeWordCheck = new QCheckBox(tr("Whole word"), this);
    m_regexCheck = new QCheckBox(tr("Regular expression"), this);
    m_wrapAroundCheck = new QCheckBox(tr("Wrap around"), this);
    m_wrapAroundCheck->setChecked(true);

    optionsLayout->addWidget(m_caseSensitiveCheck);
    optionsLayout->addWidget(m_wholeWordCheck);
    optionsLayout->addWidget(m_regexCheck);
    optionsLayout->addWidget(m_wrapAroundCheck);
    optionsLayout->addStretch();

    mainLayout->addLayout(optionsLayout);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_findNextButton = new QPushButton(tr("Find Next"), this);
    m_findNextButton->setDefault(true);
    m_findPrevButton = new QPushButton(tr("Find Previous"), this);
    m_replaceButton = new QPushButton(tr("Replace"), this);
    m_replaceAllButton = new QPushButton(tr("Replace All"), this);
    m_closeButton = new QPushButton(tr("Close"), this);

    buttonLayout->addWidget(m_findNextButton);
    buttonLayout->addWidget(m_findPrevButton);
    buttonLayout->addWidget(m_replaceButton);
    buttonLayout->addWidget(m_replaceAllButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_closeButton);

    mainLayout->addLayout(buttonLayout);

    // Status label
    m_statusLabel = new QLabel(this);
    mainLayout->addWidget(m_statusLabel);

    // Connections
    connect(m_findNextButton, &QPushButton::clicked, this, &FindDialog::onFindNext);
    connect(m_findPrevButton, &QPushButton::clicked, this, &FindDialog::onFindPrevious);
    connect(m_replaceButton, &QPushButton::clicked, this, &FindDialog::onReplace);
    connect(m_replaceAllButton, &QPushButton::clicked, this, &FindDialog::onReplaceAll);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::close);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &FindDialog::onFindNext);

    updateReplaceVisibility();
}

QString FindDialog::searchText() const
{
    return m_searchEdit->text();
}

QString FindDialog::replaceText() const
{
    return m_replaceEdit->text();
}

QTextDocument::FindFlags FindDialog::searchFlags() const
{
    QTextDocument::FindFlags flags;

    if (m_caseSensitiveCheck->isChecked()) {
        flags |= QTextDocument::FindCaseSensitively;
    }
    if (m_wholeWordCheck->isChecked()) {
        flags |= QTextDocument::FindWholeWords;
    }

    return flags;
}

void FindDialog::setReplaceMode(bool replace)
{
    m_replaceMode = replace;
    setWindowTitle(replace ? tr("Replace") : tr("Find"));
    updateReplaceVisibility();
}

void FindDialog::setSearchText(const QString &text)
{
    m_searchEdit->setText(text);
}

void FindDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    m_searchEdit->setFocus();
    m_searchEdit->selectAll();
}

void FindDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    QDialog::keyPressEvent(event);
}

void FindDialog::onFindNext()
{
    emit findNext();
}

void FindDialog::onFindPrevious()
{
    emit findPrevious();
}

void FindDialog::onReplace()
{
    emit replaceOne();
}

void FindDialog::onReplaceAll()
{
    emit replaceAll();
}

void FindDialog::updateReplaceVisibility()
{
    m_replaceLabel->setVisible(m_replaceMode);
    m_replaceEdit->setVisible(m_replaceMode);
    m_replaceButton->setVisible(m_replaceMode);
    m_replaceAllButton->setVisible(m_replaceMode);
}

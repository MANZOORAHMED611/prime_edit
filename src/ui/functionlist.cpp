#include "functionlist.h"
#include "editor.h"

#include <QVBoxLayout>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextBlock>

FunctionListPanel::FunctionListPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({tr("Functions")});

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->setHeaderHidden(false);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_treeView);

    connect(m_treeView, &QTreeView::doubleClicked,
            this, &FunctionListPanel::onDoubleClicked);

    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(1000);
    connect(&m_debounceTimer, &QTimer::timeout,
            this, &FunctionListPanel::refresh);
}

void FunctionListPanel::setEditor(Editor *editor)
{
    if (m_editor) {
        disconnect(m_editor, nullptr, &m_debounceTimer, nullptr);
    }

    m_editor = editor;

    if (m_editor) {
        connect(m_editor, &QPlainTextEdit::textChanged,
                &m_debounceTimer, qOverload<>(&QTimer::start));
    }

    refresh();
}

void FunctionListPanel::refresh()
{
    m_model->removeRows(0, m_model->rowCount());

    if (!m_editor) {
        return;
    }

    QString lang = m_editor->language().toLower();
    if (lang.isEmpty()) {
        return;
    }

    QFile file(QStringLiteral(":/syntax/%1.json").arg(lang));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull() || !doc.isObject()) {
        return;
    }

    QString regexStr = doc.object().value(
        QStringLiteral("functionRegex")).toString();
    if (regexStr.isEmpty()) {
        return;
    }

    QRegularExpression regex(
        regexStr,
        QRegularExpression::MultilineOption);
    if (!regex.isValid()) {
        return;
    }

    QString text = m_editor->toPlainText();
    QRegularExpressionMatchIterator it = regex.globalMatch(text);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        // Use first non-empty capture group (supports alternation patterns)
        QString funcName;
        for (int g = 1; g <= match.lastCapturedIndex(); ++g) {
            funcName = match.captured(g).trimmed();
            if (!funcName.isEmpty()) break;
        }
        if (funcName.isEmpty()) {
            continue;
        }

        // Calculate line number from the match offset
        int offset = match.capturedStart(0);
        int lineNumber = 1;
        for (int i = 0; i < offset && i < text.size(); ++i) {
            if (text.at(i) == QLatin1Char('\n')) {
                ++lineNumber;
            }
        }

        auto *item = new QStandardItem(funcName);
        item->setData(lineNumber, Qt::UserRole);
        item->setToolTip(tr("Line %1").arg(lineNumber));
        m_model->appendRow(item);
    }
}

void FunctionListPanel::onDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    QStandardItem *item = m_model->itemFromIndex(index);
    if (!item) {
        return;
    }

    int line = item->data(Qt::UserRole).toInt();
    emit functionActivated(line);
}

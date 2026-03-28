#include "recoverydialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

RecoveryDialog::RecoveryDialog(const QStringList &files, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Recovery"));
    auto *layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel(
        tr("PrimeEdit found unsaved documents. Recover them?")));

    m_listWidget = new QListWidget(this);
    for (const QString &filePath : files) {
        QFile f(filePath);
        QString displayName = QFileInfo(filePath).fileName();
        if (f.open(QIODevice::ReadOnly)) {
            QJsonObject obj =
                QJsonDocument::fromJson(f.readAll()).object();
            QString origPath = obj["originalFilePath"].toString();
            QString title = obj["title"].toString();
            displayName = origPath.isEmpty() ? title : origPath;
            f.close();
        }
        auto *item = new QListWidgetItem(displayName, m_listWidget);
        item->setCheckState(Qt::Checked);
        item->setData(Qt::UserRole, filePath);
    }
    layout->addWidget(m_listWidget);

    auto *btnLayout = new QHBoxLayout;
    auto *recoverBtn = new QPushButton(tr("Recover Selected"));
    auto *discardBtn = new QPushButton(tr("Discard All"));
    btnLayout->addWidget(recoverBtn);
    btnLayout->addWidget(discardBtn);
    layout->addLayout(btnLayout);

    connect(recoverBtn, &QPushButton::clicked, this, [this]() {
        m_recover = true;
        accept();
    });
    connect(discardBtn, &QPushButton::clicked, this, [this]() {
        m_recover = false;
        accept();
    });
}

QStringList RecoveryDialog::selectedFiles() const
{
    QStringList result;
    for (int i = 0; i < m_listWidget->count(); ++i) {
        QListWidgetItem *item = m_listWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            result.append(item->data(Qt::UserRole).toString());
        }
    }
    return result;
}

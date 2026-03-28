#include "plugindialog.h"
#include "core/pluginmanager.h"

#include <QTreeWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QMessageBox>

PluginDialog::PluginDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Plugins"));
    resize(620, 400);
    setupUi();
    refreshPluginList();
}

void PluginDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    // Plugin tree
    m_pluginTree = new QTreeWidget(this);
    m_pluginTree->setHeaderLabels({tr("Enabled"), tr("Name"), tr("Version"),
                                   tr("Author"), tr("Description")});
    m_pluginTree->setRootIsDecorated(false);
    m_pluginTree->setAlternatingRowColors(true);
    m_pluginTree->header()->setStretchLastSection(true);
    m_pluginTree->setColumnWidth(0, 60);
    m_pluginTree->setColumnWidth(1, 140);
    m_pluginTree->setColumnWidth(2, 60);
    m_pluginTree->setColumnWidth(3, 100);
    mainLayout->addWidget(m_pluginTree);

    connect(m_pluginTree, &QTreeWidget::itemChanged,
            this, &PluginDialog::onItemChanged);

    // Plugin directories label
    const QStringList dirs = PluginManager::instance().pluginDirectories();
    m_dirLabel = new QLabel(tr("Plugin directories:\n%1").arg(dirs.join("\n")),
                            this);
    m_dirLabel->setWordWrap(true);
    m_dirLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    mainLayout->addWidget(m_dirLabel);

    // Buttons
    auto *buttonLayout = new QHBoxLayout();
    m_openDirButton = new QPushButton(tr("Open Plugin Directory"), this);
    connect(m_openDirButton, &QPushButton::clicked,
            this, &PluginDialog::onOpenPluginDirectory);
    buttonLayout->addWidget(m_openDirButton);

    m_refreshButton = new QPushButton(tr("Refresh"), this);
    connect(m_refreshButton, &QPushButton::clicked,
            this, &PluginDialog::refreshPluginList);
    buttonLayout->addWidget(m_refreshButton);

    buttonLayout->addStretch();

    auto *closeButton = new QPushButton(tr("Close"), this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);
}

void PluginDialog::refreshPluginList()
{
    m_updating = true;
    m_pluginTree->clear();

    const auto entries = PluginManager::instance().allEntries();
    for (const auto &entry : entries) {
        auto *item = new QTreeWidgetItem(m_pluginTree);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

        bool enabled = PluginManager::instance().isPluginEnabled(entry.name);
        item->setCheckState(0, enabled ? Qt::Checked : Qt::Unchecked);
        item->setText(1, entry.name);
        item->setText(2, entry.version);
        item->setText(3, entry.author);
        item->setText(4, entry.description);
        item->setData(0, Qt::UserRole, entry.name);
    }

    if (entries.isEmpty()) {
        auto *item = new QTreeWidgetItem(m_pluginTree);
        item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
        item->setText(1, tr("No plugins found"));
        item->setDisabled(true);
    }

    m_updating = false;
}

void PluginDialog::onItemChanged(QTreeWidgetItem *item, int column)
{
    if (m_updating || column != 0) {
        return;
    }

    const QString pluginName = item->data(0, Qt::UserRole).toString();
    if (pluginName.isEmpty()) {
        return;
    }

    bool enabled = (item->checkState(0) == Qt::Checked);
    PluginManager::instance().setPluginEnabled(pluginName, enabled);

    QMessageBox::information(
        this, tr("Plugin State Changed"),
        tr("Changes to '%1' will take effect after restarting PrimeEdit.")
            .arg(pluginName));
}

void PluginDialog::onOpenPluginDirectory()
{
    // Open the first writable plugin directory, creating it if needed
    const QStringList dirs = PluginManager::instance().pluginDirectories();
    if (dirs.isEmpty()) {
        return;
    }

    // Prefer the user-config directory (second entry)
    const QString dir = (dirs.size() > 1) ? dirs.at(1) : dirs.first();
    QDir().mkpath(dir);
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

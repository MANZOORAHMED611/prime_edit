#include "remoteconnectiondialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>

// ── construction ─────────────────────────────────────────────────────

RemoteConnectionDialog::RemoteConnectionDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Open Remote File"));
    resize(820, 560);
    setupUi();
    loadSavedConnections();
}

QString RemoteConnectionDialog::selectedRemotePath() const
{
    return m_selectedRemotePath;
}

RemoteConnection::ConnectionInfo
RemoteConnectionDialog::selectedConnectionInfo() const
{
    return m_activeInfo;
}

// ── UI construction ──────────────────────────────────────────────────

void RemoteConnectionDialog::setupUi()
{
    auto *root = new QHBoxLayout(this);

    // ---------- left panel: saved connections ----------
    auto *leftPanel = new QVBoxLayout;
    auto *savedGroup = new QGroupBox(tr("Saved Connections"));
    auto *savedLayout = new QVBoxLayout(savedGroup);

    m_connectionsList = new QListWidget;
    savedLayout->addWidget(m_connectionsList);

    auto *listBtns = new QHBoxLayout;
    m_addBtn    = new QPushButton(tr("Add"));
    m_editBtn   = new QPushButton(tr("Save"));
    m_removeBtn = new QPushButton(tr("Remove"));
    listBtns->addWidget(m_addBtn);
    listBtns->addWidget(m_editBtn);
    listBtns->addWidget(m_removeBtn);
    savedLayout->addLayout(listBtns);

    leftPanel->addWidget(savedGroup);
    root->addLayout(leftPanel, 1);

    // ---------- right panel ----------
    auto *rightPanel = new QVBoxLayout;

    // -- connection form --
    auto *formGroup = new QGroupBox(tr("Connection"));
    auto *form = new QFormLayout(formGroup);

    m_nameEdit     = new QLineEdit;
    m_hostEdit     = new QLineEdit;
    m_portSpin     = new QSpinBox;
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(22);
    m_usernameEdit = new QLineEdit;

    m_authCombo = new QComboBox;
    m_authCombo->addItem(tr("SSH Key"), "key");
    m_authCombo->addItem(tr("Password (agent)"), "password");

    auto *keyRow = new QHBoxLayout;
    m_keyPathEdit  = new QLineEdit;
    m_browseKeyBtn = new QPushButton(tr("..."));
    m_browseKeyBtn->setFixedWidth(32);
    keyRow->addWidget(m_keyPathEdit);
    keyRow->addWidget(m_browseKeyBtn);

    form->addRow(tr("Name:"),     m_nameEdit);
    form->addRow(tr("Host:"),     m_hostEdit);
    form->addRow(tr("Port:"),     m_portSpin);
    form->addRow(tr("Username:"), m_usernameEdit);
    form->addRow(tr("Auth:"),     m_authCombo);
    form->addRow(tr("Key:"),      keyRow);

    rightPanel->addWidget(formGroup);

    // -- action buttons --
    auto *actionRow = new QHBoxLayout;
    m_testBtn    = new QPushButton(tr("Test Connection"));
    m_connectBtn = new QPushButton(tr("Connect"));
    m_statusLabel = new QLabel;
    actionRow->addWidget(m_testBtn);
    actionRow->addWidget(m_connectBtn);
    actionRow->addWidget(m_statusLabel, 1);
    rightPanel->addLayout(actionRow);

    // -- remote file browser --
    auto *browserGroup = new QGroupBox(tr("Remote Files"));
    auto *browserLayout = new QVBoxLayout(browserGroup);

    auto *pathRow = new QHBoxLayout;
    m_pathEdit = new QLineEdit("/");
    m_goBtn    = new QPushButton(tr("Go"));
    pathRow->addWidget(m_pathEdit, 1);
    pathRow->addWidget(m_goBtn);
    browserLayout->addLayout(pathRow);

    m_fileTree = new QTreeWidget;
    m_fileTree->setHeaderLabels({tr("Name"), tr("Type")});
    m_fileTree->header()->setStretchLastSection(true);
    m_fileTree->setColumnWidth(0, 350);
    m_fileTree->setRootIsDecorated(false);
    browserLayout->addWidget(m_fileTree, 1);

    m_openBtn = new QPushButton(tr("Open Selected File"));
    browserLayout->addWidget(m_openBtn);

    rightPanel->addWidget(browserGroup, 1);

    root->addLayout(rightPanel, 2);

    setBrowserEnabled(false);

    // ---------- signals ----------
    connect(m_addBtn, &QPushButton::clicked,
            this, &RemoteConnectionDialog::onAddConnection);
    connect(m_editBtn, &QPushButton::clicked,
            this, &RemoteConnectionDialog::onEditConnection);
    connect(m_removeBtn, &QPushButton::clicked,
            this, &RemoteConnectionDialog::onRemoveConnection);
    connect(m_connectionsList, &QListWidget::itemClicked,
            this, &RemoteConnectionDialog::onConnectionClicked);
    connect(m_testBtn, &QPushButton::clicked,
            this, &RemoteConnectionDialog::onTestConnection);
    connect(m_connectBtn, &QPushButton::clicked,
            this, &RemoteConnectionDialog::onConnect);
    connect(m_goBtn, &QPushButton::clicked, this, [this]() {
        navigateTo(m_pathEdit->text());
    });
    connect(m_fileTree, &QTreeWidget::itemDoubleClicked,
            this, &RemoteConnectionDialog::onTreeItemDoubleClicked);
    connect(m_openBtn, &QPushButton::clicked,
            this, &RemoteConnectionDialog::onOpenFile);
    connect(m_browseKeyBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(
            this, tr("Select SSH Key"),
            QDir::homePath() + "/.ssh");
        if (!path.isEmpty()) {
            m_keyPathEdit->setText(path);
        }
    });
    connect(m_authCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        bool isKey = m_authCombo->currentData().toString() == "key";
        m_keyPathEdit->setEnabled(isKey);
        m_browseKeyBtn->setEnabled(isKey);
    });
}

// ── saved connections persistence ────────────────────────────────────

void RemoteConnectionDialog::loadSavedConnections()
{
    m_connectionsList->clear();
    const auto conns = RemoteConnection::savedConnections();
    for (const auto &ci : conns) {
        m_connectionsList->addItem(ci.name);
    }
}

void RemoteConnectionDialog::onAddConnection()
{
    RemoteConnection::ConnectionInfo info = formToInfo();
    if (info.name.isEmpty()) {
        QMessageBox::warning(this, tr("Missing Name"),
                             tr("Enter a connection name first."));
        return;
    }
    RemoteConnection::saveConnection(info);
    loadSavedConnections();
}

void RemoteConnectionDialog::onEditConnection()
{
    RemoteConnection::ConnectionInfo info = formToInfo();
    if (info.name.isEmpty()) {
        QMessageBox::warning(this, tr("Missing Name"),
                             tr("Enter a connection name first."));
        return;
    }
    RemoteConnection::saveConnection(info);
    loadSavedConnections();
}

void RemoteConnectionDialog::onRemoveConnection()
{
    auto *item = m_connectionsList->currentItem();
    if (!item) return;

    RemoteConnection::removeConnection(item->text());
    loadSavedConnections();
}

void RemoteConnectionDialog::onConnectionClicked(QListWidgetItem *item)
{
    const auto conns = RemoteConnection::savedConnections();
    for (const auto &ci : conns) {
        if (ci.name == item->text()) {
            populateForm(ci);
            return;
        }
    }
}

void RemoteConnectionDialog::populateForm(
    const RemoteConnection::ConnectionInfo &info)
{
    m_nameEdit->setText(info.name);
    m_hostEdit->setText(info.host);
    m_portSpin->setValue(info.port);
    m_usernameEdit->setText(info.username);

    int authIdx = m_authCombo->findData(info.authMethod);
    if (authIdx >= 0) m_authCombo->setCurrentIndex(authIdx);

    m_keyPathEdit->setText(info.keyPath);
    m_pathEdit->setText(info.lastPath.isEmpty() ? "/" : info.lastPath);
}

RemoteConnection::ConnectionInfo
RemoteConnectionDialog::formToInfo() const
{
    RemoteConnection::ConnectionInfo info;
    info.name       = m_nameEdit->text().trimmed();
    info.host       = m_hostEdit->text().trimmed();
    info.port       = m_portSpin->value();
    info.username   = m_usernameEdit->text().trimmed();
    info.authMethod = m_authCombo->currentData().toString();
    info.keyPath    = m_keyPathEdit->text().trimmed();
    info.lastPath   = m_pathEdit->text().trimmed();
    return info;
}

// ── connection test / connect ────────────────────────────────────────

void RemoteConnectionDialog::onTestConnection()
{
    RemoteConnection::ConnectionInfo info = formToInfo();
    if (info.host.isEmpty() || info.username.isEmpty()) {
        QMessageBox::warning(this, tr("Incomplete"),
                             tr("Fill in host and username."));
        return;
    }

    m_statusLabel->setText(tr("Testing..."));
    QApplication::processEvents();

    RemoteConnection conn;
    conn.setConnectionInfo(info);
    bool ok = conn.testConnection();

    m_statusLabel->setText(ok ? tr("OK - connection succeeded")
                              : tr("FAILED - check credentials/host"));
}

void RemoteConnectionDialog::onConnect()
{
    RemoteConnection::ConnectionInfo info = formToInfo();
    if (info.host.isEmpty() || info.username.isEmpty()) {
        QMessageBox::warning(this, tr("Incomplete"),
                             tr("Fill in host and username."));
        return;
    }

    m_statusLabel->setText(tr("Connecting..."));
    QApplication::processEvents();

    // Create a persistent connection object for browsing
    if (m_connection) {
        delete m_connection;
    }
    m_connection = new RemoteConnection(this);
    m_connection->setConnectionInfo(info);
    m_activeInfo = info;

    bool ok = m_connection->testConnection();
    if (!ok) {
        m_statusLabel->setText(tr("Connection failed"));
        m_connected = false;
        setBrowserEnabled(false);
        return;
    }

    m_connected = true;
    m_statusLabel->setText(
        tr("Connected to %1").arg(info.host));
    setBrowserEnabled(true);

    // Wire up directory listing signal
    connect(m_connection, &RemoteConnection::directoryListed,
            this, [this](const QStringList &entries) {
        m_fileTree->clear();
        for (const QString &entry : entries) {
            bool isDir = entry.startsWith("d:");
            QString name = entry.mid(2);
            auto *item = new QTreeWidgetItem(m_fileTree);
            item->setText(0, name);
            item->setText(1, isDir ? tr("Directory") : tr("File"));
            item->setData(0, Qt::UserRole, isDir);
            if (isDir) {
                item->setIcon(0, style()->standardIcon(
                    QStyle::SP_DirIcon));
            } else {
                item->setIcon(0, style()->standardIcon(
                    QStyle::SP_FileIcon));
            }
        }
    });

    connect(m_connection, &RemoteConnection::connectionError,
            this, [this](const QString &err) {
        m_statusLabel->setText(tr("Error: %1").arg(err));
    });

    // Navigate to last path or /
    QString startPath = info.lastPath.isEmpty() ? "/" : info.lastPath;
    navigateTo(startPath);
}

// ── remote file browser ─────────────────────────────────────────────

void RemoteConnectionDialog::navigateTo(const QString &path)
{
    if (!m_connection || !m_connected) return;

    m_currentRemotePath = path;
    m_pathEdit->setText(path);
    m_fileTree->clear();
    m_statusLabel->setText(tr("Listing %1...").arg(path));

    m_connection->listDirectory(path);
}

void RemoteConnectionDialog::onTreeItemDoubleClicked(
    QTreeWidgetItem *item, int /*column*/)
{
    if (!item) return;

    bool isDir = item->data(0, Qt::UserRole).toBool();
    if (!isDir) {
        // Double-clicking a file selects and opens it
        onOpenFile();
        return;
    }

    // Navigate into directory
    QString dirName = item->text(0);
    QString newPath = m_currentRemotePath;
    if (!newPath.endsWith('/')) newPath += '/';
    newPath += dirName;

    navigateTo(newPath);
}

void RemoteConnectionDialog::onOpenFile()
{
    auto *item = m_fileTree->currentItem();
    if (!item) {
        QMessageBox::information(this, tr("No Selection"),
                                 tr("Select a file to open."));
        return;
    }

    bool isDir = item->data(0, Qt::UserRole).toBool();
    if (isDir) {
        // Navigate into directory instead
        onTreeItemDoubleClicked(item, 0);
        return;
    }

    QString fileName = item->text(0);
    QString remotePath = m_currentRemotePath;
    if (!remotePath.endsWith('/')) remotePath += '/';
    remotePath += fileName;

    m_selectedRemotePath = remotePath;

    // Persist the last-browsed path
    m_activeInfo.lastPath = m_currentRemotePath;
    RemoteConnection::saveConnection(m_activeInfo);

    emit fileSelected(m_activeInfo, remotePath);
    accept();
}

void RemoteConnectionDialog::setBrowserEnabled(bool enabled)
{
    m_fileTree->setEnabled(enabled);
    m_pathEdit->setEnabled(enabled);
    m_goBtn->setEnabled(enabled);
    m_openBtn->setEnabled(enabled);
}

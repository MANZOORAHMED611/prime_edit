#include "remoteconnectiondialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QInputDialog>

RemoteConnectionDialog::RemoteConnectionDialog(QWidget *parent)
    : QDialog(parent)
    , m_currentConnection(nullptr)
{
    setWindowTitle(tr("Remote Connection Manager"));
    resize(600, 500);
    setupUI();
    loadSavedConnections();
}

void RemoteConnectionDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Connection details group
    QGroupBox *detailsGroup = new QGroupBox(tr("Connection Details"));
    QFormLayout *detailsLayout = new QFormLayout(detailsGroup);

    m_hostEdit = new QLineEdit;
    m_portSpin = new QSpinBox;
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(22);

    m_usernameEdit = new QLineEdit;
    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    m_typeCombo = new QComboBox;
    m_typeCombo->addItem("SFTP", RemoteConnection::SFTP);
    m_typeCombo->addItem("SSH", RemoteConnection::SSH);
    m_typeCombo->addItem("SCP", RemoteConnection::SCP);

    detailsLayout->addRow(tr("Host:"), m_hostEdit);
    detailsLayout->addRow(tr("Port:"), m_portSpin);
    detailsLayout->addRow(tr("Username:"), m_usernameEdit);
    detailsLayout->addRow(tr("Password:"), m_passwordEdit);
    detailsLayout->addRow(tr("Type:"), m_typeCombo);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    m_connectButton = new QPushButton(tr("Connect"));
    m_disconnectButton = new QPushButton(tr("Disconnect"));
    m_disconnectButton->setEnabled(false);
    m_saveButton = new QPushButton(tr("Save Connection"));
    m_deleteButton = new QPushButton(tr("Delete Connection"));
    m_browseButton = new QPushButton(tr("Browse Files"));
    m_browseButton->setEnabled(false);

    buttonLayout->addWidget(m_connectButton);
    buttonLayout->addWidget(m_disconnectButton);
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addWidget(m_browseButton);
    buttonLayout->addStretch();

    // Saved connections group
    QGroupBox *savedGroup = new QGroupBox(tr("Saved Connections"));
    QVBoxLayout *savedLayout = new QVBoxLayout(savedGroup);

    m_connectionsList = new QListWidget;
    savedLayout->addWidget(m_connectionsList);

    // Add to main layout
    mainLayout->addWidget(detailsGroup);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(savedGroup);

    // Connect signals
    connect(m_connectButton, &QPushButton::clicked, this, &RemoteConnectionDialog::connectToServer);
    connect(m_disconnectButton, &QPushButton::clicked, this, &RemoteConnectionDialog::disconnectFromServer);
    connect(m_saveButton, &QPushButton::clicked, this, &RemoteConnectionDialog::saveConnection);
    connect(m_deleteButton, &QPushButton::clicked, this, &RemoteConnectionDialog::deleteConnection);
    connect(m_browseButton, &QPushButton::clicked, this, &RemoteConnectionDialog::browseRemoteFiles);
    connect(m_connectionsList, &QListWidget::itemDoubleClicked, this, &RemoteConnectionDialog::loadConnection);
}

void RemoteConnectionDialog::loadSavedConnections()
{
    QSettings settings;
    int size = settings.beginReadArray("RemoteConnections");

    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QString name = settings.value("name").toString();
        m_connectionsList->addItem(name);
    }

    settings.endArray();
}

void RemoteConnectionDialog::connectToServer()
{
    QString host = m_hostEdit->text();
    int port = m_portSpin->value();
    QString username = m_usernameEdit->text();
    QString password = m_passwordEdit->text();
    RemoteConnection::ConnectionType type =
        static_cast<RemoteConnection::ConnectionType>(m_typeCombo->currentData().toInt());

    if (host.isEmpty() || username.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Input"),
                           tr("Please enter host and username."));
        return;
    }

    if (m_currentConnection) {
        delete m_currentConnection;
    }

    m_currentConnection = new RemoteConnection(this);

    connect(m_currentConnection, &RemoteConnection::connected, this, [this]() {
        QMessageBox::information(this, tr("Connected"),
                               tr("Successfully connected to remote server."));
        updateConnectionStatus();
    });

    connect(m_currentConnection, &RemoteConnection::connectionError, this, [this](const QString &error) {
        QMessageBox::critical(this, tr("Connection Error"),
                            tr("Failed to connect: %1").arg(error));
        updateConnectionStatus();
    });

    m_currentConnection->connectToHost(host, port, username, password, type);
    m_connectButton->setEnabled(false);
    m_connectButton->setText(tr("Connecting..."));
}

void RemoteConnectionDialog::disconnectFromServer()
{
    if (m_currentConnection) {
        m_currentConnection->disconnect();
        delete m_currentConnection;
        m_currentConnection = nullptr;
    }

    updateConnectionStatus();
}

void RemoteConnectionDialog::saveConnection()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Save Connection"),
                                        tr("Connection name:"),
                                        QLineEdit::Normal, "", &ok);

    if (!ok || name.isEmpty()) {
        return;
    }

    QSettings settings;
    int size = settings.beginReadArray("RemoteConnections");
    settings.endArray();

    settings.beginWriteArray("RemoteConnections");
    settings.setArrayIndex(size);
    settings.setValue("name", name);
    settings.setValue("host", m_hostEdit->text());
    settings.setValue("port", m_portSpin->value());
    settings.setValue("username", m_usernameEdit->text());
    settings.setValue("type", m_typeCombo->currentIndex());
    settings.endArray();

    m_connectionsList->addItem(name);
    QMessageBox::information(this, tr("Saved"),
                           tr("Connection saved successfully."));
}

void RemoteConnectionDialog::deleteConnection()
{
    QListWidgetItem *item = m_connectionsList->currentItem();
    if (!item) {
        return;
    }

    QString name = item->text();
    int row = m_connectionsList->currentRow();

    QSettings settings;
    int size = settings.beginReadArray("RemoteConnections");
    QVector<QMap<QString, QVariant>> connections;

    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        if (i != row) {
            QMap<QString, QVariant> conn;
            conn["name"] = settings.value("name");
            conn["host"] = settings.value("host");
            conn["port"] = settings.value("port");
            conn["username"] = settings.value("username");
            conn["type"] = settings.value("type");
            connections.append(conn);
        }
    }
    settings.endArray();

    settings.beginWriteArray("RemoteConnections");
    for (int i = 0; i < connections.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("name", connections[i]["name"]);
        settings.setValue("host", connections[i]["host"]);
        settings.setValue("port", connections[i]["port"]);
        settings.setValue("username", connections[i]["username"]);
        settings.setValue("type", connections[i]["type"]);
    }
    settings.endArray();

    delete item;
    QMessageBox::information(this, tr("Deleted"),
                           tr("Connection deleted successfully."));
}

void RemoteConnectionDialog::loadConnection()
{
    QListWidgetItem *item = m_connectionsList->currentItem();
    if (!item) {
        return;
    }

    int row = m_connectionsList->currentRow();
    QSettings settings;
    settings.beginReadArray("RemoteConnections");
    settings.setArrayIndex(row);

    m_hostEdit->setText(settings.value("host").toString());
    m_portSpin->setValue(settings.value("port").toInt());
    m_usernameEdit->setText(settings.value("username").toString());
    m_typeCombo->setCurrentIndex(settings.value("type").toInt());

    settings.endArray();
}

void RemoteConnectionDialog::browseRemoteFiles()
{
    if (!m_currentConnection || m_currentConnection->status() != RemoteConnection::Connected) {
        QMessageBox::warning(this, tr("Not Connected"),
                           tr("Please connect to a server first."));
        return;
    }

    // This would open a file browser dialog for remote files
    // For now, just show a message
    QMessageBox::information(this, tr("Browse Files"),
                           tr("Remote file browser not yet implemented."));
}

void RemoteConnectionDialog::updateConnectionStatus()
{
    bool connected = m_currentConnection &&
                    m_currentConnection->status() == RemoteConnection::Connected;

    m_connectButton->setEnabled(!connected);
    m_connectButton->setText(tr("Connect"));
    m_disconnectButton->setEnabled(connected);
    m_browseButton->setEnabled(connected);
}

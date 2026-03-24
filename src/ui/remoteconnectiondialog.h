#ifndef REMOTECONNECTIONDIALOG_H
#define REMOTECONNECTIONDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QListWidget>
#include "../core/remoteconnection.h"

class RemoteConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RemoteConnectionDialog(QWidget *parent = nullptr);
    ~RemoteConnectionDialog() override = default;

private slots:
    void connectToServer();
    void disconnectFromServer();
    void saveConnection();
    void deleteConnection();
    void loadConnection();
    void browseRemoteFiles();

private:
    void setupUI();
    void loadSavedConnections();
    void updateConnectionStatus();

    // Connection details
    QLineEdit *m_hostEdit;
    QSpinBox *m_portSpin;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QComboBox *m_typeCombo;

    // Buttons
    QPushButton *m_connectButton;
    QPushButton *m_disconnectButton;
    QPushButton *m_saveButton;
    QPushButton *m_deleteButton;
    QPushButton *m_browseButton;

    // Saved connections
    QListWidget *m_connectionsList;

    // Current connection
    RemoteConnection *m_currentConnection;
    QString m_currentConnectionName;
};

#endif // REMOTECONNECTIONDIALOG_H

#ifndef REMOTECONNECTIONDIALOG_H
#define REMOTECONNECTIONDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QListWidget>
#include <QTreeWidget>
#include <QLabel>
#include "../core/remoteconnection.h"

/**
 * Dialog for managing SSH connections and browsing/opening remote files.
 *
 * Layout:
 *   Left panel  - saved connections list + Add/Edit/Remove buttons
 *   Right panel - connection form, Test/Connect, remote file browser, Open
 *
 * Emits fileSelected() when the user picks a remote file to open.
 */
class RemoteConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RemoteConnectionDialog(QWidget *parent = nullptr);
    ~RemoteConnectionDialog() override = default;

    /** The remote path the user chose (empty if cancelled). */
    QString selectedRemotePath() const;

    /** The connection info for the selected file. */
    RemoteConnection::ConnectionInfo selectedConnectionInfo() const;

signals:
    /** Emitted when the user clicks Open on a remote file. */
    void fileSelected(const RemoteConnection::ConnectionInfo &info,
                      const QString &remotePath);

private slots:
    void onAddConnection();
    void onEditConnection();
    void onRemoveConnection();
    void onConnectionClicked(QListWidgetItem *item);
    void onTestConnection();
    void onConnect();
    void onTreeItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onOpenFile();

private:
    void setupUi();
    void loadSavedConnections();
    void populateForm(const RemoteConnection::ConnectionInfo &info);
    RemoteConnection::ConnectionInfo formToInfo() const;
    void navigateTo(const QString &path);
    void setBrowserEnabled(bool enabled);

    // Left panel
    QListWidget *m_connectionsList = nullptr;
    QPushButton *m_addBtn = nullptr;
    QPushButton *m_editBtn = nullptr;
    QPushButton *m_removeBtn = nullptr;

    // Right panel - form
    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_hostEdit = nullptr;
    QSpinBox  *m_portSpin = nullptr;
    QLineEdit *m_usernameEdit = nullptr;
    QComboBox *m_authCombo = nullptr;
    QLineEdit *m_keyPathEdit = nullptr;
    QPushButton *m_browseKeyBtn = nullptr;
    QLabel *m_statusLabel = nullptr;

    // Right panel - actions
    QPushButton *m_testBtn = nullptr;
    QPushButton *m_connectBtn = nullptr;

    // Right panel - file browser
    QTreeWidget *m_fileTree = nullptr;
    QLineEdit *m_pathEdit = nullptr;
    QPushButton *m_goBtn = nullptr;
    QPushButton *m_openBtn = nullptr;

    // State
    RemoteConnection *m_connection = nullptr;
    RemoteConnection::ConnectionInfo m_activeInfo;
    QString m_currentRemotePath;
    QString m_selectedRemotePath;
    bool m_connected = false;
};

#endif // REMOTECONNECTIONDIALOG_H

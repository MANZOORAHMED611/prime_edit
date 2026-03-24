#ifndef TERMINALWIDGET_H
#define TERMINALWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QProcess>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QComboBox>
#include <QToolButton>
#include <QStringList>

class TerminalWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget *parent = nullptr);
    ~TerminalWidget() override;

    // Terminal control
    void startShell(const QString &shell = QString());
    void executeCommand(const QString &command);
    void clear();
    void stop();

    // Directory
    void setWorkingDirectory(const QString &dir);
    QString workingDirectory() const { return m_workingDir; }

signals:
    void commandExecuted(const QString &command);
    void processFinished(int exitCode);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onInputReturnPressed();
    void onShellChanged(int index);
    void onClearClicked();
    void onStopClicked();

private:
    void setupUi();
    void appendOutput(const QString &text, const QColor &color = QColor());
    void executeInShell(const QString &command);
    void addToHistory(const QString &command);
    void navigateHistory(int direction);

    QTextEdit *m_outputEdit;
    QLineEdit *m_inputEdit;
    QComboBox *m_shellCombo;
    QToolButton *m_clearBtn;
    QToolButton *m_stopBtn;

    QProcess *m_process;
    QString m_workingDir;
    QStringList m_commandHistory;
    int m_historyIndex;
    QString m_currentShell;
};

#endif // TERMINALWIDGET_H

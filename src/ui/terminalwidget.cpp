#include "terminalwidget.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QScrollBar>
#include <QDir>
#include <QFont>
#include <QFontDatabase>

TerminalWidget::TerminalWidget(QWidget *parent)
    : QWidget(parent)
    , m_process(nullptr)
    , m_historyIndex(-1)
{
    setupUi();

    // Set default working directory
    m_workingDir = QDir::currentPath();

    // Start default shell
    startShell();
}

TerminalWidget::~TerminalWidget()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(1000);
        delete m_process;
    }
}

void TerminalWidget::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    // Toolbar
    QHBoxLayout *toolbarLayout = new QHBoxLayout();
    toolbarLayout->setContentsMargins(4, 4, 4, 4);
    toolbarLayout->setSpacing(4);

    QLabel *shellLabel = new QLabel(tr("Shell:"), this);
    toolbarLayout->addWidget(shellLabel);

    m_shellCombo = new QComboBox(this);
    m_shellCombo->addItem("bash");
    m_shellCombo->addItem("sh");
    m_shellCombo->addItem("zsh");
    m_shellCombo->addItem("fish");
    connect(m_shellCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TerminalWidget::onShellChanged);
    toolbarLayout->addWidget(m_shellCombo);

    toolbarLayout->addStretch();

    m_clearBtn = new QToolButton(this);
    m_clearBtn->setText(tr("Clear"));
    m_clearBtn->setToolTip(tr("Clear terminal output"));
    connect(m_clearBtn, &QToolButton::clicked, this, &TerminalWidget::onClearClicked);
    toolbarLayout->addWidget(m_clearBtn);

    m_stopBtn = new QToolButton(this);
    m_stopBtn->setText(tr("Stop"));
    m_stopBtn->setToolTip(tr("Stop current process"));
    connect(m_stopBtn, &QToolButton::clicked, this, &TerminalWidget::onStopClicked);
    toolbarLayout->addWidget(m_stopBtn);

    layout->addLayout(toolbarLayout);

    // Output area
    m_outputEdit = new QTextEdit(this);
    m_outputEdit->setReadOnly(true);
    m_outputEdit->setUndoRedoEnabled(false);

    // Set monospace font
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(10);
    m_outputEdit->setFont(font);

    // Dark terminal theme
    m_outputEdit->setStyleSheet(
        "QTextEdit {"
        "    background-color: #1e1e1e;"
        "    color: #d4d4d4;"
        "    border: none;"
        "}"
    );

    layout->addWidget(m_outputEdit, 1);

    // Input area
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->setContentsMargins(4, 4, 4, 4);
    inputLayout->setSpacing(4);

    QLabel *promptLabel = new QLabel(tr("$"), this);
    promptLabel->setStyleSheet("QLabel { color: #4ec9b0; font-weight: bold; }");
    inputLayout->addWidget(promptLabel);

    m_inputEdit = new QLineEdit(this);
    m_inputEdit->setFont(font);
    m_inputEdit->setStyleSheet(
        "QLineEdit {"
        "    background-color: #2d2d2d;"
        "    color: #d4d4d4;"
        "    border: 1px solid #3e3e3e;"
        "    padding: 4px;"
        "}"
    );
    m_inputEdit->installEventFilter(this);
    connect(m_inputEdit, &QLineEdit::returnPressed, this, &TerminalWidget::onInputReturnPressed);
    inputLayout->addWidget(m_inputEdit);

    layout->addLayout(inputLayout);

    setLayout(layout);
}

void TerminalWidget::startShell(const QString &shell)
{
    // Stop existing process
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(1000);
        delete m_process;
    }

    // Create new process
    m_process = new QProcess(this);
    m_process->setWorkingDirectory(m_workingDir);

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &TerminalWidget::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &TerminalWidget::onReadyReadStandardError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TerminalWidget::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &TerminalWidget::onProcessError);

    // Determine shell to use
    m_currentShell = shell.isEmpty() ? m_shellCombo->currentText() : shell;

    // Welcome message
    appendOutput(QString("PrimeEdit Terminal\n"), QColor(100, 150, 255));
    appendOutput(QString("Working Directory: %1\n").arg(m_workingDir), QColor(150, 150, 150));
    appendOutput(QString("Shell: %1\n\n").arg(m_currentShell), QColor(150, 150, 150));
}

void TerminalWidget::executeCommand(const QString &command)
{
    if (command.isEmpty()) return;

    // Show command in output
    appendOutput(QString("$ %1\n").arg(command), QColor(78, 201, 176));

    // Add to history
    addToHistory(command);

    // Execute command
    executeInShell(command);
}

void TerminalWidget::executeInShell(const QString &command)
{
    if (!m_process) {
        appendOutput("Error: No shell process running\n", QColor(255, 100, 100));
        return;
    }

    // Check for cd command (need to handle specially)
    QString trimmed = command.trimmed();
    if (trimmed.startsWith("cd ")) {
        QString newDir = trimmed.mid(3).trimmed();

        // Remove quotes if present
        if (newDir.startsWith('"') && newDir.endsWith('"')) {
            newDir = newDir.mid(1, newDir.length() - 2);
        } else if (newDir.startsWith('\'') && newDir.endsWith('\'')) {
            newDir = newDir.mid(1, newDir.length() - 2);
        }

        // Expand ~ to home directory
        if (newDir == "~" || newDir.startsWith("~/")) {
            newDir.replace(0, 1, QDir::homePath());
        }

        // Handle relative paths
        if (!QDir::isAbsolutePath(newDir)) {
            newDir = QDir(m_workingDir).absoluteFilePath(newDir);
        }

        QDir dir(newDir);
        if (dir.exists()) {
            m_workingDir = dir.absolutePath();
            if (m_process) {
                m_process->setWorkingDirectory(m_workingDir);
            }
            appendOutput(QString("Changed directory to: %1\n").arg(m_workingDir), QColor(150, 150, 150));
        } else {
            appendOutput(QString("cd: %1: No such file or directory\n").arg(newDir), QColor(255, 100, 100));
        }
        return;
    }

    // Check for clear command
    if (trimmed == "clear" || trimmed == "cls") {
        clear();
        return;
    }

    // Execute command in shell
    QStringList args;
    args << "-c" << command;

    m_process->start(m_currentShell, args);
    m_process->waitForFinished(-1);  // Wait indefinitely for command to complete
}

void TerminalWidget::clear()
{
    m_outputEdit->clear();
}

void TerminalWidget::stop()
{
    if (m_process && m_process->state() == QProcess::Running) {
        m_process->kill();
        appendOutput("\nProcess stopped\n", QColor(255, 200, 100));
    }
}

void TerminalWidget::setWorkingDirectory(const QString &dir)
{
    QDir d(dir);
    if (d.exists()) {
        m_workingDir = d.absolutePath();
        if (m_process) {
            m_process->setWorkingDirectory(m_workingDir);
        }
    }
}

void TerminalWidget::onReadyReadStandardOutput()
{
    if (m_process) {
        QString output = QString::fromUtf8(m_process->readAllStandardOutput());
        appendOutput(output, QColor(212, 212, 212));
    }
}

void TerminalWidget::onReadyReadStandardError()
{
    if (m_process) {
        QString output = QString::fromUtf8(m_process->readAllStandardError());
        appendOutput(output, QColor(255, 100, 100));
    }
}

void TerminalWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::CrashExit) {
        appendOutput(QString("\nProcess crashed (exit code: %1)\n").arg(exitCode), QColor(255, 100, 100));
    } else if (exitCode != 0) {
        appendOutput(QString("\nProcess exited with code %1\n").arg(exitCode), QColor(255, 200, 100));
    }

    emit processFinished(exitCode);
}

void TerminalWidget::onProcessError(QProcess::ProcessError error)
{
    QString errorStr;
    switch (error) {
    case QProcess::FailedToStart:
        errorStr = "Failed to start process";
        break;
    case QProcess::Crashed:
        errorStr = "Process crashed";
        break;
    case QProcess::Timedout:
        errorStr = "Process timed out";
        break;
    case QProcess::WriteError:
        errorStr = "Write error";
        break;
    case QProcess::ReadError:
        errorStr = "Read error";
        break;
    default:
        errorStr = "Unknown error";
        break;
    }

    appendOutput(QString("Error: %1\n").arg(errorStr), QColor(255, 100, 100));
}

void TerminalWidget::onInputReturnPressed()
{
    QString command = m_inputEdit->text();
    m_inputEdit->clear();
    m_historyIndex = -1;

    executeCommand(command);
    emit commandExecuted(command);
}

void TerminalWidget::onShellChanged(int index)
{
    Q_UNUSED(index);
    startShell(m_shellCombo->currentText());
}

void TerminalWidget::onClearClicked()
{
    clear();
}

void TerminalWidget::onStopClicked()
{
    stop();
}

void TerminalWidget::appendOutput(const QString &text, const QColor &color)
{
    QTextCursor cursor = m_outputEdit->textCursor();
    cursor.movePosition(QTextCursor::End);

    if (color.isValid()) {
        QTextCharFormat format;
        format.setForeground(color);
        cursor.setCharFormat(format);
    }

    cursor.insertText(text);
    m_outputEdit->setTextCursor(cursor);

    // Auto-scroll to bottom
    m_outputEdit->verticalScrollBar()->setValue(
        m_outputEdit->verticalScrollBar()->maximum()
    );
}

void TerminalWidget::addToHistory(const QString &command)
{
    if (command.isEmpty()) return;

    // Don't add duplicates
    if (!m_commandHistory.isEmpty() && m_commandHistory.last() == command) {
        return;
    }

    m_commandHistory.append(command);

    // Limit history size
    if (m_commandHistory.size() > 100) {
        m_commandHistory.removeFirst();
    }
}

void TerminalWidget::navigateHistory(int direction)
{
    if (m_commandHistory.isEmpty()) return;

    if (direction < 0) {  // Up arrow - previous command
        if (m_historyIndex == -1) {
            m_historyIndex = m_commandHistory.size() - 1;
        } else if (m_historyIndex > 0) {
            --m_historyIndex;
        }
    } else {  // Down arrow - next command
        if (m_historyIndex >= 0 && m_historyIndex < m_commandHistory.size() - 1) {
            ++m_historyIndex;
        } else {
            m_historyIndex = -1;
            m_inputEdit->clear();
            return;
        }
    }

    if (m_historyIndex >= 0 && m_historyIndex < m_commandHistory.size()) {
        m_inputEdit->setText(m_commandHistory[m_historyIndex]);
    }
}

bool TerminalWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_inputEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Up) {
            navigateHistory(-1);
            return true;
        } else if (keyEvent->key() == Qt::Key_Down) {
            navigateHistory(1);
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}

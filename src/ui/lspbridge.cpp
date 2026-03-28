#include "lspbridge.h"
#include "mainwindow.h"
#include "editor.h"
#include "completionpopup.h"
#include "core/document.h"
#include "core/lspmanager.h"
#include "core/lspclient.h"

#include <QDir>
#include <QFileInfo>
#include <QToolTip>
#include <QCursor>

LSPBridge::LSPBridge(MainWindow *mainWindow, QObject *parent)
    : QObject(parent), m_mainWindow(mainWindow)
{
    m_changeTimer.setSingleShot(true);
    m_changeTimer.setInterval(300);
    connect(&m_changeTimer, &QTimer::timeout,
            this, &LSPBridge::onTextChanged);
}

QString LSPBridge::fileUri(Editor *editor) const
{
    if (!editor || !editor->document()) return QString();
    QString path = editor->document()->filePath();
    if (path.isEmpty()) return QString();
    return QStringLiteral("file://") + path;
}

QString LSPBridge::languageId(Editor *editor) const
{
    if (!editor || !editor->document()) return QString();
    QString lang = editor->document()->language().toLower();
    if (lang == "c++" || lang == "cpp") return QStringLiteral("cpp");
    if (lang == "c") return QStringLiteral("c");
    if (lang == "javascript") return QStringLiteral("javascript");
    if (lang == "typescript") return QStringLiteral("typescript");
    if (lang == "python") return QStringLiteral("python");
    if (lang == "rust") return QStringLiteral("rust");
    if (lang == "go") return QStringLiteral("go");
    if (lang == "java") return QStringLiteral("java");
    return lang;
}

QString LSPBridge::rootPath() const
{
    if (m_activeEditor && m_activeEditor->document()
        && !m_activeEditor->document()->filePath().isEmpty()) {
        return QFileInfo(m_activeEditor->document()->filePath())
            .absolutePath();
    }
    return QDir::homePath();
}

LSPClient *LSPBridge::clientForEditor(Editor *editor)
{
    if (!editor || !editor->document()) return nullptr;
    QString lang = editor->document()->language();
    return LSPManager::instance().clientForLanguage(lang);
}

void LSPBridge::onEditorOpened(Editor *editor)
{
    if (!editor || !editor->document()) return;

    QString lang = editor->document()->language();
    if (!LSPManager::instance().hasServerForLanguage(lang)) return;

    LSPManager::instance().startServer(lang, rootPath());

    LSPClient *client = LSPManager::instance().clientForLanguage(lang);
    if (!client) return;

    auto connectWhenReady = [this, editor, client]() {
        QString uri = fileUri(editor);
        if (uri.isEmpty()) return;
        client->didOpen(uri, languageId(editor),
                        editor->toPlainText());

        // Diagnostics
        connect(client, &LSPClient::diagnosticsPublished,
                editor, [editor, this](const QString &diagUri,
                                       const QVector<Diagnostic> &diagnostics) {
            if (diagUri == fileUri(editor)) {
                editor->setDiagnostics(diagnostics);
            }
        }, Qt::UniqueConnection);

        // Hover
        connect(client, &LSPClient::hoverResult,
                editor, [editor](const QString &content) {
            if (!content.isEmpty()) {
                QToolTip::showText(QCursor::pos(), content, editor);
            }
        }, Qt::UniqueConnection);

        // Definition
        connect(client, &LSPClient::definitionResult,
                this, [this](const Location &location) {
            QString path = location.uri;
            if (path.startsWith("file://")) path = path.mid(7);
            if (path.isEmpty()) return;
            m_mainWindow->openFile(path);
            Editor *e = m_mainWindow->currentEditor();
            if (e) e->goToLine(location.line + 1);
        }, Qt::UniqueConnection);

        // Completion
        connect(client, &LSPClient::completionResult,
                editor, [editor](const QVector<CompletionItem> &items) {
            if (!items.isEmpty()) {
                editor->showLSPCompletions(items);
            }
        }, Qt::UniqueConnection);

        // References (handled by MainWindow via signal)
        connect(client, &LSPClient::referencesResult,
                this, [](const QVector<Location> &locations) {
            Q_UNUSED(locations);
        }, Qt::UniqueConnection);
    };

    if (client->isInitialized()) {
        connectWhenReady();
    } else {
        connect(client, &LSPClient::initialized,
                this, connectWhenReady, Qt::SingleShotConnection);
    }

    m_activeEditor = editor;
}

void LSPBridge::onEditorClosed(Editor *editor)
{
    LSPClient *client = clientForEditor(editor);
    if (!client) return;

    QString uri = fileUri(editor);
    if (!uri.isEmpty()) {
        client->didClose(uri);
    }
}

void LSPBridge::onEditorChanged(Editor *editor)
{
    m_activeEditor = editor;
    m_changeTimer.start();
}

void LSPBridge::onEditorSaved(Editor *editor)
{
    LSPClient *client = clientForEditor(editor);
    if (!client) return;

    QString uri = fileUri(editor);
    if (!uri.isEmpty()) {
        client->didSave(uri);
    }
}

void LSPBridge::onTextChanged()
{
    if (!m_activeEditor) return;
    LSPClient *client = clientForEditor(m_activeEditor);
    if (!client) return;

    QString uri = fileUri(m_activeEditor);
    if (!uri.isEmpty()) {
        client->didChange(uri, m_activeEditor->toPlainText());
    }
}

// Notepad++ Edit > Insert and Edit > Copy to Clipboard.
#include "mainwindow.h"
#include "editor.h"
#include "core/document.h"
#include "core/texttransforms.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QMenu>
#include <QStatusBar>

// Notepad++ ships these without default shortcuts.
void MainWindow::setupInsertAndClipboardMenus()
{
    QMenu *insertMenu = m_editMenu->addMenu(tr("Insert"));
    insertMenu->addAction(tr("Date Time (short)"), this, [this]() { insertDateTime(false); });
    insertMenu->addAction(tr("Date Time (long)"), this, [this]() { insertDateTime(true); });

    QMenu *copyMenu = m_editMenu->addMenu(tr("Copy to Clipboard"));
    copyMenu->addAction(tr("Current Full File Path to Clipboard"), this,
                        [this]() { copyPathToClipboard(PathPart::FullPath); });
    copyMenu->addAction(tr("Current Filename to Clipboard"), this,
                        [this]() { copyPathToClipboard(PathPart::FileName); });
    copyMenu->addAction(tr("Current Dir. Path to Clipboard"), this,
                        [this]() { copyPathToClipboard(PathPart::Directory); });
}

// Replaces the selection, or inserts at the cursor, as one undo step.
void MainWindow::insertDateTime(bool longFormat)
{
    Editor *editor = currentEditor();
    if (!editor)
        return;
    if (editor->isReadOnly()) {
        showNotification(tr("'%1' is read-only, so it was not changed.")
                             .arg(editor->document()->displayName()));
        return;
    }
    const QString stamp = TextTransforms::formatDateTime(QDateTime::currentDateTime(),
                                                         QLocale::system(), longFormat);
    const QTextCursor cursor = editor->textCursor();
    editor->replaceRange(cursor.selectionStart(), cursor.selectionEnd(), stamp, false);
}

void MainWindow::copyPathToClipboard(PathPart part)
{
    Editor *editor = currentEditor();
    if (!editor)
        return;
    const Document *doc = editor->document();
    if (doc->isUntitled() && part != PathPart::FileName) {
        showNotification(tr("'%1' has not been saved, so it has no path.").arg(doc->displayName()));
        return;
    }
    const QFileInfo info(doc->filePath());
    QString text;
    switch (part) {
    case PathPart::FullPath: text = QDir::toNativeSeparators(info.absoluteFilePath()); break;
    case PathPart::FileName: text = doc->isUntitled() ? doc->displayName() : info.fileName(); break;
    case PathPart::Directory: text = QDir::toNativeSeparators(info.absolutePath()); break;
    }
    QApplication::clipboard()->setText(text);
    statusBar()->showMessage(tr("Copied %1").arg(text), 3000);
}

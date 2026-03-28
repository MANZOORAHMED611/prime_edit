#ifndef LSPBRIDGE_H
#define LSPBRIDGE_H

#include <QObject>
#include <QTimer>

class MainWindow;
class Editor;
class LSPClient;

class LSPBridge : public QObject
{
    Q_OBJECT
public:
    explicit LSPBridge(MainWindow *mainWindow, QObject *parent = nullptr);

    void onEditorOpened(Editor *editor);
    void onEditorClosed(Editor *editor);
    void onEditorChanged(Editor *editor);
    void onEditorSaved(Editor *editor);

    LSPClient *clientForEditor(Editor *editor);

private slots:
    void onTextChanged();

private:
    QString fileUri(Editor *editor) const;
    QString languageId(Editor *editor) const;
    QString rootPath() const;

    MainWindow *m_mainWindow;
    QTimer m_changeTimer;
    Editor *m_activeEditor = nullptr;
};

#endif // LSPBRIDGE_H

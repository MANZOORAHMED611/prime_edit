#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QStatusBar>
#include <QLabel>
#include <QPushButton>
#include <QMenu>

class Editor;
class MainWindow;

class StatusBarWidget : public QStatusBar
{
    Q_OBJECT

public:
    explicit StatusBarWidget(MainWindow *parent = nullptr);

    void updateFromEditor(Editor *editor);
    void clear();

    void setDocType(const QString &type);
    void setLength(qint64 length, int lines);
    void setPosition(int line, int column, int selCount, qint64 pos);
    void setEncoding(const QString &encoding);
    void setLineEnding(const QString &lineEnding);
    void setLanguage(const QString &language);
    void setInsertMode(bool insert);

private:
    void setupEolMenu();
    void setupEncodingMenu();

    MainWindow *m_mainWindow;

    QLabel *m_docTypeLabel;
    QLabel *m_lengthLabel;
    QLabel *m_positionLabel;
    QPushButton *m_eolButton;
    QPushButton *m_encodingButton;
    QLabel *m_insertModeLabel;

    QMenu *m_eolMenu;
    QMenu *m_encodingMenu;
};

#endif // STATUSBAR_H

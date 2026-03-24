#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QStatusBar>
#include <QLabel>

class Editor;

class StatusBarWidget : public QStatusBar
{
    Q_OBJECT

public:
    explicit StatusBarWidget(QWidget *parent = nullptr);

    void updateFromEditor(Editor *editor);

    void setPosition(int line, int column);
    void setEncoding(const QString &encoding);
    void setLineEnding(const QString &lineEnding);
    void setLanguage(const QString &language);
    void setFileInfo(const QString &info);

private:
    QLabel *m_positionLabel;
    QLabel *m_encodingLabel;
    QLabel *m_lineEndingLabel;
    QLabel *m_languageLabel;
    QLabel *m_infoLabel;
};

#endif // STATUSBAR_H

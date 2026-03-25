#ifndef LINENUMBERAREA_H
#define LINENUMBERAREA_H

#include <QWidget>
#include <QMouseEvent>

class Editor;

class LineNumberArea : public QWidget
{
    Q_OBJECT

public:
    explicit LineNumberArea(Editor *editor);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    Editor *m_editor;
};

#endif // LINENUMBERAREA_H

#ifndef DOCUMENTMAP_H
#define DOCUMENTMAP_H

#include <QWidget>
#include <QPixmap>
#include <QTimer>

class Editor;

class DocumentMapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DocumentMapWidget(QWidget *parent = nullptr);
    void setEditor(Editor *editor);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void regenerateMap();
    void onScrollChanged();

private:
    void navigateToY(int y);

    Editor *m_editor = nullptr;
    QPixmap m_cachedMap;
    QTimer m_debounceTimer;
    int m_lineHeight = 2;
};

#endif // DOCUMENTMAP_H

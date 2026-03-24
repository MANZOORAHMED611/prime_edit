#ifndef HEXVIEWER_H
#define HEXVIEWER_H

#include <QWidget>
#include <QScrollArea>
#include <QByteArray>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFont>

class HexViewerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HexViewerWidget(QWidget *parent = nullptr);
    ~HexViewerWidget() override = default;

    void setData(const QByteArray &data);
    QByteArray data() const { return m_data; }

    void setBytesPerLine(int bytes);
    int bytesPerLine() const { return m_bytesPerLine; }

    void setShowAscii(bool show);
    bool showAscii() const { return m_showAscii; }

    void setShowOffsets(bool show);
    bool showOffsets() const { return m_showOffsets; }

    qint64 cursorPosition() const { return m_cursorPos; }
    void setCursorPosition(qint64 pos);

    qint64 selectionStart() const { return m_selectionStart; }
    qint64 selectionEnd() const { return m_selectionEnd; }
    void setSelection(qint64 start, qint64 end);
    void clearSelection();

signals:
    void cursorPositionChanged(qint64 pos);
    void selectionChanged(qint64 start, qint64 end);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QSize sizeHint() const override;
    void updateGeometry();
    qint64 positionFromPoint(const QPoint &point);
    QRect byteRect(qint64 pos);
    void ensureCursorVisible();

    QByteArray m_data;
    int m_bytesPerLine;
    bool m_showAscii;
    bool m_showOffsets;
    qint64 m_cursorPos;
    qint64 m_selectionStart;
    qint64 m_selectionEnd;
    int m_lineHeight;
    int m_charWidth;
    int m_offsetWidth;
    int m_hexWidth;
    int m_asciiWidth;
    QFont m_font;
};

class HexViewer : public QWidget
{
    Q_OBJECT

public:
    explicit HexViewer(QWidget *parent = nullptr);
    ~HexViewer() override = default;

    void loadFile(const QString &filePath);
    void setData(const QByteArray &data);
    QByteArray data() const;

    void setBytesPerLine(int bytes);
    void setShowAscii(bool show);
    void setShowOffsets(bool show);

public slots:
    void goToOffset(qint64 offset);
    void find(const QByteArray &pattern);
    void exportSelection();

private:
    void setupUI();

    HexViewerWidget *m_hexWidget;
    QScrollArea *m_scrollArea;
};

#endif // HEXVIEWER_H

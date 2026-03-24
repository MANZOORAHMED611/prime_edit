#include "hexviewer.h"
#include <QPainter>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QFontMetrics>
#include <QFile>
#include <QMessageBox>
#include <QFileDialog>
#include <QKeyEvent>
#include <QApplication>
#include <QClipboard>

// HexViewerWidget implementation
HexViewerWidget::HexViewerWidget(QWidget *parent)
    : QWidget(parent)
    , m_bytesPerLine(16)
    , m_showAscii(true)
    , m_showOffsets(true)
    , m_cursorPos(0)
    , m_selectionStart(-1)
    , m_selectionEnd(-1)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    m_font = QFont("Courier New", 10);
    m_font.setStyleHint(QFont::Monospace);
    m_font.setFixedPitch(true);

    QFontMetrics fm(m_font);
    m_charWidth = fm.horizontalAdvance('0');
    m_lineHeight = fm.height();

    updateGeometry();
}

void HexViewerWidget::setData(const QByteArray &data)
{
    m_data = data;
    m_cursorPos = 0;
    m_selectionStart = -1;
    m_selectionEnd = -1;
    updateGeometry();
    update();
}

void HexViewerWidget::setBytesPerLine(int bytes)
{
    m_bytesPerLine = bytes;
    updateGeometry();
    update();
}

void HexViewerWidget::setShowAscii(bool show)
{
    m_showAscii = show;
    updateGeometry();
    update();
}

void HexViewerWidget::setShowOffsets(bool show)
{
    m_showOffsets = show;
    updateGeometry();
    update();
}

void HexViewerWidget::setCursorPosition(qint64 pos)
{
    if (pos < 0 || pos >= m_data.size()) {
        return;
    }

    m_cursorPos = pos;
    emit cursorPositionChanged(pos);
    ensureCursorVisible();
    update();
}

void HexViewerWidget::setSelection(qint64 start, qint64 end)
{
    m_selectionStart = start;
    m_selectionEnd = end;
    emit selectionChanged(start, end);
    update();
}

void HexViewerWidget::clearSelection()
{
    m_selectionStart = -1;
    m_selectionEnd = -1;
    emit selectionChanged(-1, -1);
    update();
}

void HexViewerWidget::updateGeometry()
{
    QFontMetrics fm(m_font);
    m_charWidth = fm.horizontalAdvance('0');
    m_lineHeight = fm.height();

    // Calculate widths
    m_offsetWidth = m_showOffsets ? (8 * m_charWidth + 10) : 0;  // "00000000: "
    m_hexWidth = m_bytesPerLine * 3 * m_charWidth;  // Each byte is "XX "
    m_asciiWidth = m_showAscii ? (m_bytesPerLine * m_charWidth + 10) : 0;

    int totalWidth = m_offsetWidth + m_hexWidth + m_asciiWidth + 20;
    int totalHeight = ((m_data.size() + m_bytesPerLine - 1) / m_bytesPerLine) * m_lineHeight + 20;

    setMinimumSize(totalWidth, totalHeight);
    resize(totalWidth, totalHeight);
}

QSize HexViewerWidget::sizeHint() const
{
    int totalWidth = m_offsetWidth + m_hexWidth + m_asciiWidth + 20;
    int totalHeight = ((m_data.size() + m_bytesPerLine - 1) / m_bytesPerLine) * m_lineHeight + 20;
    return QSize(totalWidth, totalHeight);
}

void HexViewerWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setFont(m_font);

    QRect rect = event->rect();
    painter.fillRect(rect, Qt::white);

    if (m_data.isEmpty()) {
        return;
    }

    int lineCount = (m_data.size() + m_bytesPerLine - 1) / m_bytesPerLine;
    int firstLine = qMax(0, (rect.top() - 10) / m_lineHeight);
    int lastLine = qMin(lineCount - 1, (rect.bottom() - 10) / m_lineHeight + 1);

    int xOffset = 10;
    int xHex = xOffset + m_offsetWidth;
    int xAscii = xHex + m_hexWidth + 10;

    for (int line = firstLine; line <= lastLine; ++line) {
        int yPos = 10 + line * m_lineHeight + m_lineHeight - 3;
        qint64 lineOffset = line * m_bytesPerLine;

        // Draw offset
        if (m_showOffsets) {
            painter.setPen(Qt::darkGray);
            QString offset = QString("%1:").arg(lineOffset, 8, 16, QChar('0')).toUpper();
            painter.drawText(xOffset, yPos, offset);
        }

        // Draw hex bytes
        for (int i = 0; i < m_bytesPerLine; ++i) {
            qint64 pos = lineOffset + i;
            if (pos >= m_data.size()) {
                break;
            }

            int xBytePos = xHex + i * 3 * m_charWidth;

            // Highlight selection
            if (m_selectionStart >= 0 && m_selectionEnd >= 0 &&
                pos >= m_selectionStart && pos <= m_selectionEnd) {
                QRect highlightRect(xBytePos, yPos - m_lineHeight + 3,
                                   2 * m_charWidth, m_lineHeight);
                painter.fillRect(highlightRect, QColor(173, 216, 230));
            }

            // Highlight cursor
            if (pos == m_cursorPos) {
                QRect cursorRect(xBytePos, yPos - m_lineHeight + 3,
                               2 * m_charWidth, m_lineHeight);
                painter.fillRect(cursorRect, QColor(255, 255, 200));
            }

            unsigned char byte = static_cast<unsigned char>(m_data[pos]);
            QString hexByte = QString("%1").arg(byte, 2, 16, QChar('0')).toUpper();

            painter.setPen(Qt::black);
            painter.drawText(xBytePos, yPos, hexByte);
        }

        // Draw ASCII
        if (m_showAscii) {
            for (int i = 0; i < m_bytesPerLine; ++i) {
                qint64 pos = lineOffset + i;
                if (pos >= m_data.size()) {
                    break;
                }

                int xCharPos = xAscii + i * m_charWidth;

                // Highlight selection in ASCII
                if (m_selectionStart >= 0 && m_selectionEnd >= 0 &&
                    pos >= m_selectionStart && pos <= m_selectionEnd) {
                    QRect highlightRect(xCharPos, yPos - m_lineHeight + 3,
                                       m_charWidth, m_lineHeight);
                    painter.fillRect(highlightRect, QColor(173, 216, 230));
                }

                unsigned char byte = static_cast<unsigned char>(m_data[pos]);
                char ch = (byte >= 32 && byte < 127) ? byte : '.';

                painter.setPen(Qt::darkBlue);
                painter.drawText(xCharPos, yPos, QString(QChar(ch)));
            }
        }
    }
}

qint64 HexViewerWidget::positionFromPoint(const QPoint &point)
{
    int line = (point.y() - 10) / m_lineHeight;
    if (line < 0) return 0;

    qint64 lineOffset = line * m_bytesPerLine;

    int xHex = 10 + m_offsetWidth;
    int relX = point.x() - xHex;

    if (relX >= 0 && relX < m_hexWidth) {
        int byteIndex = relX / (3 * m_charWidth);
        qint64 pos = lineOffset + byteIndex;
        return qMin(pos, (qint64)m_data.size() - 1);
    }

    return qMin(lineOffset, (qint64)m_data.size() - 1);
}

void HexViewerWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        qint64 pos = positionFromPoint(event->pos());
        m_cursorPos = pos;

        if (!(event->modifiers() & Qt::ShiftModifier)) {
            clearSelection();
            m_selectionStart = pos;
        }

        emit cursorPositionChanged(pos);
        update();
    }
}

void HexViewerWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        qint64 pos = positionFromPoint(event->pos());
        m_selectionEnd = pos;
        m_cursorPos = pos;
        emit cursorPositionChanged(pos);
        emit selectionChanged(m_selectionStart, m_selectionEnd);
        update();
    }
}

void HexViewerWidget::wheelEvent(QWheelEvent *event)
{
    QWidget::wheelEvent(event);
}

void HexViewerWidget::keyPressEvent(QKeyEvent *event)
{
    qint64 newPos = m_cursorPos;

    switch (event->key()) {
    case Qt::Key_Left:
        newPos = qMax(0LL, m_cursorPos - 1);
        break;
    case Qt::Key_Right:
        newPos = qMin((qint64)m_data.size() - 1, m_cursorPos + 1);
        break;
    case Qt::Key_Up:
        newPos = qMax(0LL, m_cursorPos - m_bytesPerLine);
        break;
    case Qt::Key_Down:
        newPos = qMin((qint64)m_data.size() - 1, m_cursorPos + m_bytesPerLine);
        break;
    case Qt::Key_Home:
        newPos = 0;
        break;
    case Qt::Key_End:
        newPos = m_data.size() - 1;
        break;
    case Qt::Key_PageUp:
        newPos = qMax(0LL, m_cursorPos - m_bytesPerLine * 10);
        break;
    case Qt::Key_PageDown:
        newPos = qMin((qint64)m_data.size() - 1, m_cursorPos + m_bytesPerLine * 10);
        break;
    case Qt::Key_C:
        if (event->modifiers() & Qt::ControlModifier) {
            if (m_selectionStart >= 0 && m_selectionEnd >= 0) {
                qint64 start = qMin(m_selectionStart, m_selectionEnd);
                qint64 end = qMax(m_selectionStart, m_selectionEnd);
                QByteArray selected = m_data.mid(start, end - start + 1);
                QApplication::clipboard()->setText(selected.toHex());
            }
        }
        break;
    default:
        QWidget::keyPressEvent(event);
        return;
    }

    if (newPos != m_cursorPos) {
        m_cursorPos = newPos;
        emit cursorPositionChanged(newPos);
        ensureCursorVisible();
        update();
    }
}

QRect HexViewerWidget::byteRect(qint64 pos)
{
    int line = pos / m_bytesPerLine;
    int col = pos % m_bytesPerLine;

    int xHex = 10 + m_offsetWidth;
    int xBytePos = xHex + col * 3 * m_charWidth;
    int yPos = 10 + line * m_lineHeight;

    return QRect(xBytePos, yPos, 2 * m_charWidth, m_lineHeight);
}

void HexViewerWidget::ensureCursorVisible()
{
    QRect rect = byteRect(m_cursorPos);

    QWidget *parent = parentWidget();
    if (QScrollArea *scrollArea = qobject_cast<QScrollArea*>(parent)) {
        scrollArea->ensureVisible(rect.center().x(), rect.center().y(),
                                 rect.width() / 2, rect.height() / 2);
    }
}

// HexViewer implementation
HexViewer::HexViewer(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void HexViewer::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(false);

    m_hexWidget = new HexViewerWidget(this);
    m_scrollArea->setWidget(m_hexWidget);

    layout->addWidget(m_scrollArea);
}

void HexViewer::loadFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"),
                           tr("Could not open file: %1").arg(filePath));
        return;
    }

    QByteArray data = file.readAll();
    m_hexWidget->setData(data);
}

void HexViewer::setData(const QByteArray &data)
{
    m_hexWidget->setData(data);
}

QByteArray HexViewer::data() const
{
    return m_hexWidget->data();
}

void HexViewer::setBytesPerLine(int bytes)
{
    m_hexWidget->setBytesPerLine(bytes);
}

void HexViewer::setShowAscii(bool show)
{
    m_hexWidget->setShowAscii(show);
}

void HexViewer::setShowOffsets(bool show)
{
    m_hexWidget->setShowOffsets(show);
}

void HexViewer::goToOffset(qint64 offset)
{
    m_hexWidget->setCursorPosition(offset);
}

void HexViewer::find(const QByteArray &pattern)
{
    QByteArray data = m_hexWidget->data();
    qint64 startPos = m_hexWidget->cursorPosition() + 1;

    qint64 foundPos = data.indexOf(pattern, startPos);
    if (foundPos >= 0) {
        m_hexWidget->setCursorPosition(foundPos);
        m_hexWidget->setSelection(foundPos, foundPos + pattern.size() - 1);
    } else {
        QMessageBox::information(this, tr("Find"), tr("Pattern not found."));
    }
}

void HexViewer::exportSelection()
{
    qint64 start = m_hexWidget->selectionStart();
    qint64 end = m_hexWidget->selectionEnd();

    if (start < 0 || end < 0) {
        QMessageBox::warning(this, tr("Export"), tr("No selection to export."));
        return;
    }

    qint64 begin = qMin(start, end);
    qint64 finish = qMax(start, end);

    QString filePath = QFileDialog::getSaveFileName(this, tr("Export Selection"));
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Error"),
                           tr("Could not write file: %1").arg(filePath));
        return;
    }

    QByteArray data = m_hexWidget->data();
    QByteArray selected = data.mid(begin, finish - begin + 1);
    file.write(selected);

    QMessageBox::information(this, tr("Export"),
                           tr("Selection exported successfully."));
}

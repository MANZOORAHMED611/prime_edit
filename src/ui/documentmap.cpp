#include "documentmap.h"
#include "editor.h"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextDocument>

DocumentMapWidget::DocumentMapWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumWidth(100);
    setMaximumWidth(150);

    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(500);
    connect(&m_debounceTimer, &QTimer::timeout,
            this, &DocumentMapWidget::regenerateMap);
}

void DocumentMapWidget::setEditor(Editor *editor)
{
    if (m_editor) {
        disconnect(m_editor, nullptr, this, nullptr);
        QScrollBar *vsb = m_editor->verticalScrollBar();
        if (vsb) {
            disconnect(vsb, nullptr, this, nullptr);
        }
    }

    m_editor = editor;

    if (m_editor) {
        connect(m_editor, &Editor::textChanged,
                &m_debounceTimer, qOverload<>(&QTimer::start));
        QScrollBar *vsb = m_editor->verticalScrollBar();
        if (vsb) {
            connect(vsb, &QScrollBar::valueChanged,
                    this, &DocumentMapWidget::onScrollChanged);
        }
        regenerateMap();
    } else {
        m_cachedMap = QPixmap();
        update();
    }
}

void DocumentMapWidget::regenerateMap()
{
    if (!m_editor) {
        m_cachedMap = QPixmap();
        update();
        return;
    }

    QTextDocument *doc = static_cast<QPlainTextEdit*>(m_editor)->document();
    int lineCount = doc->blockCount();
    if (lineCount <= 0) {
        lineCount = 1;
    }

    int mapWidth = width();
    int mapHeight = lineCount * m_lineHeight;
    if (mapHeight < 1) {
        mapHeight = 1;
    }

    QPixmap pixmap(mapWidth, mapHeight);
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#606060"));

    QTextBlock block = doc->begin();
    int y = 0;
    while (block.isValid()) {
        int textLen = block.text().length();
        if (textLen > 0) {
            int stripWidth = qMin(textLen, mapWidth);
            painter.drawRect(0, y, stripWidth, m_lineHeight);
        }
        y += m_lineHeight;
        block = block.next();
    }

    painter.end();
    m_cachedMap = pixmap;
    update();
}

void DocumentMapWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    if (m_cachedMap.isNull() || !m_editor) {
        return;
    }

    // Draw the cached map scaled to widget height
    painter.drawPixmap(rect(), m_cachedMap);

    // Draw viewport rectangle
    QScrollBar *vsb = m_editor->verticalScrollBar();
    if (!vsb) {
        return;
    }

    QTextDocument *doc = static_cast<QPlainTextEdit*>(m_editor)->document();
    int totalLines = doc->blockCount();
    if (totalLines <= 0) {
        totalLines = 1;
    }

    int visibleLines = m_editor->height() / m_editor->fontMetrics().lineSpacing();
    if (visibleLines < 1) {
        visibleLines = 1;
    }

    double firstVisibleLine = vsb->value();
    double lineRatio = static_cast<double>(height()) / totalLines;

    int vpTop = static_cast<int>(firstVisibleLine * lineRatio);
    int vpHeight = static_cast<int>(visibleLines * lineRatio);
    if (vpHeight < 4) {
        vpHeight = 4;
    }

    QColor vpColor(70, 130, 200, 80);
    painter.fillRect(0, vpTop, width(), vpHeight, vpColor);

    QColor borderColor(70, 130, 200, 160);
    painter.setPen(borderColor);
    painter.drawRect(0, vpTop, width() - 1, vpHeight);
}

void DocumentMapWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        navigateToY(event->pos().y());
    }
}

void DocumentMapWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        navigateToY(event->pos().y());
    }
}

void DocumentMapWidget::navigateToY(int y)
{
    if (!m_editor) {
        return;
    }

    QTextDocument *doc = static_cast<QPlainTextEdit*>(m_editor)->document();
    int totalLines = doc->blockCount();
    if (totalLines <= 0) {
        return;
    }

    double ratio = static_cast<double>(y) / height();
    int targetLine = static_cast<int>(ratio * totalLines) + 1;
    targetLine = qBound(1, targetLine, totalLines);

    m_editor->goToLine(targetLine);
}

void DocumentMapWidget::onScrollChanged()
{
    update();
}

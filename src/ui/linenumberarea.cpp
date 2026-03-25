#include "linenumberarea.h"
#include "editor.h"

#include <QMouseEvent>
#include <QTextBlock>

LineNumberArea::LineNumberArea(Editor *editor)
    : QWidget(editor)
    , m_editor(editor)
{
}

QSize LineNumberArea::sizeHint() const
{
    return QSize(m_editor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    m_editor->lineNumberAreaPaintEvent(event);
}

void LineNumberArea::mousePressEvent(QMouseEvent *event)
{
    int y = event->pos().y();
    QTextBlock block = m_editor->firstVisibleBlock();
    int top = qRound(m_editor->blockBoundingGeometry(block)
                     .translated(m_editor->contentOffset()).top());

    while (block.isValid()) {
        int bottom = top + qRound(m_editor->blockBoundingRect(block).height());
        if (y >= top && y < bottom) {
            int line = block.blockNumber() + 1;
            int x = event->pos().x();

            if (x < m_editor->bookmarkMarginWidth()) {
                m_editor->toggleBookmark(line);
            }
            return;
        }
        top = bottom;
        block = block.next();
    }

    QWidget::mousePressEvent(event);
}

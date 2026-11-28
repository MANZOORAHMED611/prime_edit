#include "markdownoutline.h"

#include <QHeaderView>
#include <QRegularExpression>

namespace {
const QRegularExpression kAtx(QStringLiteral("^ {0,3}(#{1,6})(?:[ \\t]+(.*?))?(?:[ \\t]+#+)?[ \\t]*$"));
const QRegularExpression kSetext1(QStringLiteral("^ {0,3}=+[ \\t]*$"));
const QRegularExpression kSetext2(QStringLiteral("^ {0,3}-+[ \\t]*$"));
const QRegularExpression kFence(QStringLiteral("^ {0,3}(```|~~~)"));
constexpr int kLineRole = Qt::UserRole;
constexpr int kIndexRole = Qt::UserRole + 1;
}

MarkdownOutline::MarkdownOutline(QWidget *parent)
    : QTreeWidget(parent)
{
    setHeaderHidden(true);
    setColumnCount(1);
    setUniformRowHeights(true);
    setExpandsOnDoubleClick(false);
    header()->setStretchLastSection(true);
    connect(this, &QTreeWidget::itemClicked, this,
            [this](QTreeWidgetItem *item) { onItemActivated(item); });
    connect(this, &QTreeWidget::itemActivated, this,
            [this](QTreeWidgetItem *item) { onItemActivated(item); });
}

QList<MarkdownOutline::Heading> MarkdownOutline::parseHeadings(const QString &markdown)
{
    QList<Heading> headings;
    const QStringList lines = markdown.split(QLatin1Char('\n'));
    QString fence;
    for (int i = 0; i < lines.size(); ++i) {
        const QString &line = lines.at(i);
        const auto fenceMatch = kFence.match(line);
        if (fenceMatch.hasMatch()) {
            const QString marker = fenceMatch.captured(1);
            if (fence.isEmpty()) {
                fence = marker;
            } else if (marker == fence) {
                fence.clear();
            }
            continue;
        }
        if (!fence.isEmpty()) {
            continue;
        }
        if (const auto atx = kAtx.match(line); atx.hasMatch()) {
            headings.append({static_cast<int>(atx.captured(1).size()), i + 1,
                             stripInline(atx.captured(2))});
            continue;
        }
        const bool prevIsText = i > 0 && !lines.at(i - 1).trimmed().isEmpty()
            && (headings.isEmpty() || headings.last().line != i);
        if (prevIsText && (kSetext1.match(line).hasMatch() || kSetext2.match(line).hasMatch())) {
            const int level = kSetext1.match(line).hasMatch() ? 1 : 2;
            headings.append({level, i, stripInline(lines.at(i - 1).trimmed())});
        }
    }
    return headings;
}

QString MarkdownOutline::stripInline(const QString &title)
{
    QString t = title;
    t.replace(QRegularExpression(QStringLiteral("!?\\[([^\\]]*)\\]\\([^)]*\\)")), QStringLiteral("\\1"));
    t.remove(QRegularExpression(QStringLiteral("[*_`~]")));
    return t.trimmed().isEmpty() ? tr("(untitled)") : t.trimmed();
}

void MarkdownOutline::setMarkdown(const QString &markdown)
{
    const QList<Heading> headings = parseHeadings(markdown);
    clear();
    m_items.clear();
    m_headings = headings;
    QList<QTreeWidgetItem *> stack; // stack[k] = latest item at depth k
    QList<int> levels;
    for (int i = 0; i < headings.size(); ++i) {
        const Heading &h = headings.at(i);
        while (!levels.isEmpty() && levels.last() >= h.level) {
            levels.removeLast();
            stack.removeLast();
        }
        auto *item = stack.isEmpty() ? new QTreeWidgetItem(this)
                                     : new QTreeWidgetItem(stack.last());
        item->setText(0, h.title);
        item->setToolTip(0, h.title);
        item->setData(0, kLineRole, h.line);
        item->setData(0, kIndexRole, i);
        stack.append(item);
        levels.append(h.level);
        m_items.append(item);
    }
    expandAll();
}

void MarkdownOutline::setCurrentLine(int line)
{
    int current = -1;
    for (int i = 0; i < m_headings.size() && m_headings.at(i).line <= line; ++i) {
        current = i;
    }
    m_syncingSelection = true;
    if (current >= 0) {
        setCurrentItem(m_items.at(current));
        scrollToItem(m_items.at(current));
    } else {
        clearSelection();
    }
    m_syncingSelection = false;
}

void MarkdownOutline::onItemActivated(QTreeWidgetItem *item)
{
    if (!item || m_syncingSelection) {
        return;
    }
    emit headingActivated(item->data(0, kLineRole).toInt(), item->data(0, kIndexRole).toInt());
}

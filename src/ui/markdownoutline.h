#ifndef MARKDOWNOUTLINE_H
#define MARKDOWNOUTLINE_H

#include <QList>
#include <QString>
#include <QTreeWidget>

// Heading tree for a Markdown document, used as a left navigation panel.
class MarkdownOutline : public QTreeWidget
{
    Q_OBJECT

public:
    struct Heading {
        int level = 1;  // 1..6
        int line = 1;   // 1-based source line of the heading text
        QString title;  // inline markup stripped
    };

    explicit MarkdownOutline(QWidget *parent = nullptr);

    // ATX (# ...) and setext (=== / ---) headings, skipping fenced code.
    static QList<Heading> parseHeadings(const QString &markdown);

    void setMarkdown(const QString &markdown);
    // Selects the heading whose section contains this 1-based line.
    void setCurrentLine(int line);

signals:
    // headingIndex counts headings in document order (matches the preview).
    void headingActivated(int line, int headingIndex);

private:
    void onItemActivated(QTreeWidgetItem *item);
    static QString stripInline(const QString &title);

    QList<Heading> m_headings;
    QList<QTreeWidgetItem *> m_items; // parallel to m_headings
    bool m_syncingSelection = false;
};

#endif // MARKDOWNOUTLINE_H

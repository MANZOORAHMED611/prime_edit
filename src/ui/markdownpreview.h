#ifndef MARKDOWNPREVIEW_H
#define MARKDOWNPREVIEW_H

#include <QTextBrowser>
#include <QString>
#include <QUrl>

// Read-only rendered view of Markdown source, using Qt's built-in GitHub
// dialect (tables, task lists, fenced code, strikethrough). QTextBrowser runs
// no scripts; loadResource() additionally restricts resources to local image
// files so a document can never trigger network access.
class QTextBlock;

class MarkdownPreview : public QTextBrowser
{
    Q_OBJECT

public:
    explicit MarkdownPreview(QWidget *parent = nullptr);

    // baseDir resolves relative image paths and relative links.
    void render(const QString &markdown, const QString &baseDir);
    // Scrolls so the Nth heading (document order, 0-based) is at the top.
    void scrollToHeading(int headingIndex);

signals:
    // A link to a local file (e.g. another .md) the editor should open.
    void openFileRequested(const QString &path);

protected:
    QVariant loadResource(int type, const QUrl &name) override;

private:
    void onAnchorClicked(const QUrl &url);
    void polishDocument(); // table padding, code-block shading
    void shadeInlineCode(const QTextBlock &block, const QColor &shade);

    QString m_baseDir;
};

#endif // MARKDOWNPREVIEW_H

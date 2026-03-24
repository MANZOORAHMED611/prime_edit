#include "markdownpreview.h"
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QScrollBar>

MarkdownPreview::MarkdownPreview(QWidget *parent)
    : QWidget(parent)
    , m_autoUpdate(true)
    , m_updateDelay(500)
    , m_zoomFactor(1.0)
{
    setupUI();

    m_updateTimer = new QTimer(this);
    m_updateTimer->setSingleShot(true);
    connect(m_updateTimer, &QTimer::timeout, this, &MarkdownPreview::updatePreview);
}

void MarkdownPreview::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_browser = new QTextBrowser(this);
    m_browser->setOpenExternalLinks(true);
    m_browser->setReadOnly(true);

    layout->addWidget(m_browser);
}

void MarkdownPreview::setMarkdown(const QString &markdown)
{
    m_markdown = markdown;

    if (m_autoUpdate) {
        m_updateTimer->start(m_updateDelay);
    }
}

void MarkdownPreview::updatePreview()
{
    // Save scroll position
    int scrollPos = m_browser->verticalScrollBar()->value();

    QString html = markdownToHtml(m_markdown);
    m_browser->setHtml(html);

    // Restore scroll position
    m_browser->verticalScrollBar()->setValue(scrollPos);
}

void MarkdownPreview::refresh()
{
    updatePreview();
}

void MarkdownPreview::setAutoUpdate(bool enabled)
{
    m_autoUpdate = enabled;
    if (enabled) {
        updatePreview();
    }
}

void MarkdownPreview::setUpdateDelay(int ms)
{
    m_updateDelay = ms;
    m_updateTimer->setInterval(ms);
}

void MarkdownPreview::zoomIn()
{
    m_zoomFactor *= 1.1;
    QFont font = m_browser->font();
    font.setPointSizeF(font.pointSizeF() * 1.1);
    m_browser->setFont(font);
}

void MarkdownPreview::zoomOut()
{
    m_zoomFactor /= 1.1;
    QFont font = m_browser->font();
    font.setPointSizeF(font.pointSizeF() / 1.1);
    m_browser->setFont(font);
}

void MarkdownPreview::resetZoom()
{
    m_zoomFactor = 1.0;
    QFont font = m_browser->font();
    font.setPointSizeF(10.0); // Default size
    m_browser->setFont(font);
}

QString MarkdownPreview::generateCSS()
{
    return R"(
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', 'Roboto', 'Oxygen', 'Ubuntu', 'Cantarell', 'Fira Sans', 'Droid Sans', 'Helvetica Neue', sans-serif;
            line-height: 1.6;
            padding: 20px;
            max-width: 900px;
            margin: 0 auto;
            color: #333;
            background-color: #fff;
        }

        h1, h2, h3, h4, h5, h6 {
            margin-top: 24px;
            margin-bottom: 16px;
            font-weight: 600;
            line-height: 1.25;
        }

        h1 {
            font-size: 2em;
            border-bottom: 1px solid #eee;
            padding-bottom: 0.3em;
        }

        h2 {
            font-size: 1.5em;
            border-bottom: 1px solid #eee;
            padding-bottom: 0.3em;
        }

        h3 { font-size: 1.25em; }
        h4 { font-size: 1em; }
        h5 { font-size: 0.875em; }
        h6 { font-size: 0.85em; color: #6a737d; }

        p {
            margin-top: 0;
            margin-bottom: 16px;
        }

        a {
            color: #0366d6;
            text-decoration: none;
        }

        a:hover {
            text-decoration: underline;
        }

        code {
            background-color: rgba(27, 31, 35, 0.05);
            border-radius: 3px;
            font-family: 'SFMono-Regular', 'Consolas', 'Liberation Mono', 'Menlo', monospace;
            font-size: 85%;
            margin: 0;
            padding: 0.2em 0.4em;
        }

        pre {
            background-color: #f6f8fa;
            border-radius: 3px;
            font-family: 'SFMono-Regular', 'Consolas', 'Liberation Mono', 'Menlo', monospace;
            font-size: 85%;
            line-height: 1.45;
            overflow: auto;
            padding: 16px;
        }

        pre code {
            background-color: transparent;
            border: 0;
            display: inline;
            line-height: inherit;
            margin: 0;
            overflow: visible;
            padding: 0;
            word-wrap: normal;
        }

        blockquote {
            border-left: 4px solid #dfe2e5;
            color: #6a737d;
            margin: 0 0 16px 0;
            padding: 0 1em;
        }

        ul, ol {
            margin-bottom: 16px;
            padding-left: 2em;
        }

        li {
            margin-bottom: 0.25em;
        }

        table {
            border-collapse: collapse;
            border-spacing: 0;
            margin-bottom: 16px;
            width: 100%;
        }

        table th {
            background-color: #f6f8fa;
            font-weight: 600;
            padding: 6px 13px;
            border: 1px solid #dfe2e5;
        }

        table td {
            padding: 6px 13px;
            border: 1px solid #dfe2e5;
        }

        table tr {
            background-color: #fff;
            border-top: 1px solid #c6cbd1;
        }

        table tr:nth-child(2n) {
            background-color: #f6f8fa;
        }

        img {
            max-width: 100%;
            box-sizing: content-box;
        }

        hr {
            border: 0;
            border-top: 1px solid #e1e4e8;
            height: 0;
            margin: 24px 0;
        }

        input[type="checkbox"] {
            margin-right: 0.5em;
        }
    )";
}

QString MarkdownPreview::markdownToHtml(const QString &markdown)
{
    QString html = markdown;

    // Escape HTML entities
    html.replace("&", "&amp;");
    html.replace("<", "&lt;");
    html.replace(">", "&gt;");

    // Headers (must be before other patterns)
    html.replace(QRegularExpression("^######\\s+(.+)$", QRegularExpression::MultilineOption), "<h6>\\1</h6>");
    html.replace(QRegularExpression("^#####\\s+(.+)$", QRegularExpression::MultilineOption), "<h5>\\1</h5>");
    html.replace(QRegularExpression("^####\\s+(.+)$", QRegularExpression::MultilineOption), "<h4>\\1</h4>");
    html.replace(QRegularExpression("^###\\s+(.+)$", QRegularExpression::MultilineOption), "<h3>\\1</h3>");
    html.replace(QRegularExpression("^##\\s+(.+)$", QRegularExpression::MultilineOption), "<h2>\\1</h2>");
    html.replace(QRegularExpression("^#\\s+(.+)$", QRegularExpression::MultilineOption), "<h1>\\1</h1>");

    // Code blocks (fenced with ```)
    QRegularExpression codeBlockRegex("```([\\s\\S]*?)```");
    html.replace(codeBlockRegex, "<pre><code>\\1</code></pre>");

    // Inline code
    html.replace(QRegularExpression("`([^`]+)`"), "<code>\\1</code>");

    // Bold
    html.replace(QRegularExpression("\\*\\*([^*]+)\\*\\*"), "<strong>\\1</strong>");
    html.replace(QRegularExpression("__([^_]+)__"), "<strong>\\1</strong>");

    // Italic
    html.replace(QRegularExpression("\\*([^*]+)\\*"), "<em>\\1</em>");
    html.replace(QRegularExpression("_([^_]+)_"), "<em>\\1</em>");

    // Strikethrough
    html.replace(QRegularExpression("~~([^~]+)~~"), "<del>\\1</del>");

    // Links [text](url)
    html.replace(QRegularExpression("\\[([^\\]]+)\\]\\(([^)]+)\\)"), "<a href=\"\\2\">\\1</a>");

    // Images ![alt](url)
    html.replace(QRegularExpression("!\\[([^\\]]*)\\]\\(([^)]+)\\)"), "<img src=\"\\2\" alt=\"\\1\" />");

    // Blockquotes
    html.replace(QRegularExpression("^>\\s+(.+)$", QRegularExpression::MultilineOption), "<blockquote>\\1</blockquote>");

    // Horizontal rules
    html.replace(QRegularExpression("^---$", QRegularExpression::MultilineOption), "<hr />");
    html.replace(QRegularExpression("^\\*\\*\\*$", QRegularExpression::MultilineOption), "<hr />");

    // Unordered lists
    QStringList lines = html.split('\n');
    QStringList processedLines;
    bool inUl = false;
    bool inOl = false;

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();

        // Unordered list items
        if (trimmed.startsWith("- ") || trimmed.startsWith("* ")) {
            if (!inUl) {
                processedLines.append("<ul>");
                inUl = true;
            }
            QString item = trimmed.mid(2);
            processedLines.append("<li>" + item + "</li>");
        }
        // Ordered list items
        else if (QRegularExpression("^\\d+\\.\\s+").match(trimmed).hasMatch()) {
            if (!inOl) {
                processedLines.append("<ol>");
                inOl = true;
            }
            QString item = trimmed.mid(trimmed.indexOf('.') + 1).trimmed();
            processedLines.append("<li>" + item + "</li>");
        }
        else {
            if (inUl) {
                processedLines.append("</ul>");
                inUl = false;
            }
            if (inOl) {
                processedLines.append("</ol>");
                inOl = false;
            }
            processedLines.append(line);
        }
    }

    if (inUl) processedLines.append("</ul>");
    if (inOl) processedLines.append("</ol>");

    html = processedLines.join('\n');

    // Paragraphs (wrap non-tagged lines)
    lines = html.split('\n');
    processedLines.clear();

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (!trimmed.isEmpty() &&
            !trimmed.startsWith('<') &&
            !trimmed.endsWith('>')) {
            processedLines.append("<p>" + line + "</p>");
        } else {
            processedLines.append(line);
        }
    }

    html = processedLines.join('\n');

    // Wrap in HTML document
    QString css = generateCSS();
    QString fullHtml = QString(
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<meta charset=\"utf-8\">\n"
        "<style>\n%1\n</style>\n"
        "</head>\n"
        "<body>\n%2\n</body>\n"
        "</html>"
    ).arg(css, html);

    return fullHtml;
}

#include "markdownpreview.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QFontDatabase>
#include <QTextCursor>
#include <QTextFrame>
#include <QTextTable>
#include <QTextBlock>
#include <QTextDocument>

MarkdownPreview::MarkdownPreview(QWidget *parent)
    : QTextBrowser(parent)
{
    setReadOnly(true);
    // Never navigate inside the preview: links are routed by onAnchorClicked.
    setOpenLinks(false);
    setOpenExternalLinks(false);
    document()->setDocumentMargin(16);
    // As an overlay this widget is a child of the (monospace) editor; reading
    // text should use the normal UI font, not inherit the code font.
    setFont(QApplication::font());
    connect(this, &QTextBrowser::anchorClicked, this, &MarkdownPreview::onAnchorClicked);
}

void MarkdownPreview::render(const QString &markdown, const QString &baseDir)
{
    // Keep the reading position across live re-renders while typing.
    QScrollBar *bar = verticalScrollBar();
    const double fraction = bar->maximum() > 0
        ? static_cast<double>(bar->value()) / bar->maximum() : 0.0;

    m_baseDir = baseDir;
    if (!baseDir.isEmpty()) {
        document()->setBaseUrl(QUrl::fromLocalFile(baseDir + QDir::separator()));
    }
    document()->setMarkdown(markdown, QTextDocument::MarkdownDialectGitHub);
    polishDocument();

    bar->setValue(static_cast<int>(fraction * bar->maximum()));
}

void MarkdownPreview::polishDocument()
{
    const QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    const QColor shade = palette().color(QPalette::AlternateBase);
    for (QTextBlock block = document()->begin(); block.isValid(); block = block.next()) {
        QTextBlockFormat fmt = block.blockFormat();
        if (fmt.hasProperty(QTextFormat::BlockCodeFence)) {
            QTextCursor cursor(block);
            fmt.setBackground(shade);
            fmt.setLeftMargin(8);
            cursor.setBlockFormat(fmt);
            cursor.select(QTextCursor::BlockUnderCursor);
            QTextCharFormat chars;
            chars.setFont(mono);
            cursor.mergeCharFormat(chars);
        }
        if (QTextTable *table = QTextCursor(block).currentTable()) {
            QTextTableFormat tf = table->format();
            if (tf.cellPadding() < 4) {
                tf.setCellPadding(5);
                tf.setCellSpacing(0);
                tf.setBorderCollapse(true);
                table->setFormat(tf);
            }
        }
    }
}

void MarkdownPreview::scrollToHeading(int headingIndex)
{
    int seen = -1;
    for (QTextBlock block = document()->begin(); block.isValid(); block = block.next()) {
        if (block.blockFormat().headingLevel() > 0 && ++seen == headingIndex) {
            const QRectF rect = document()->documentLayout()->blockBoundingRect(block);
            verticalScrollBar()->setValue(static_cast<int>(rect.top()));
            return;
        }
    }
}

QVariant MarkdownPreview::loadResource(int type, const QUrl &name)
{
    if (type != QTextDocument::ImageResource) {
        return {};
    }
    const QUrl resolved = document()->baseUrl().resolved(name);
    if (!resolved.isLocalFile()) {
        return {}; // no network fetches from document content
    }
    const QString path = resolved.toLocalFile();
    if (QImageReader::imageFormat(path).isEmpty()) {
        return {}; // only real images, never arbitrary files
    }
    return QTextBrowser::loadResource(type, resolved);
}

void MarkdownPreview::onAnchorClicked(const QUrl &url)
{
    if (url.scheme().isEmpty() && url.path().isEmpty() && url.hasFragment()) {
        scrollToAnchor(url.fragment());
        return;
    }
    const QString scheme = url.scheme().toLower();
    if (scheme == "http" || scheme == "https" || scheme == "mailto") {
        QDesktopServices::openUrl(url);
        return;
    }
    const QUrl resolved = document()->baseUrl().resolved(url);
    if (resolved.isLocalFile() && QFileInfo(resolved.toLocalFile()).isFile()) {
        emit openFileRequested(resolved.toLocalFile());
    }
}

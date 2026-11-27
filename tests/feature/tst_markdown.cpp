// Markdown preview modes and outline panel.

#include "test_isolation.h"

#include "ui/editor.h"
#include "ui/mainwindow.h"
#include "ui/markdownmode.h"
#include "ui/markdownoutline.h"
#include "ui/markdownpreview.h"
#include "utils/settings.h"

#include <QApplication>
#include <QDockWidget>
#include <QFontDatabase>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTest>
#include <QTextBlock>

// Exposes the protected resource hook for the network-blocking test.
struct PreviewProbe : MarkdownPreview {
    using MarkdownPreview::loadResource;
};

class TestMarkdown : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    QString writeFile(const QString &name, const QByteArray &bytes);
    static MarkdownPreview *overlayOf(Editor *editor);

private slots:
    void initTestCase();
    void parsesAtxAndSetextHeadings();
    void ignoresHeadingsInFencedCode();
    void stripsInlineMarkupFromTitles();
    void outlineNestsByLevel();
    void previewRendersGithubTables();
    void previewBlocksRemoteImages();
    void previewStylesInlineCode();
    void previewScrollsToHeading();
    void cycleSourceSideRenderedSource();
    void cycleIsNoOpForNonMarkdown();
    void outlineFollowsMarkdownTabsOnly();
    void outlineClickGoesToHeadingLine();
    void escapeLeavesRenderedMode();
    void livePreviewUpdatesWhileTyping();
};

QString TestMarkdown::writeFile(const QString &name, const QByteArray &bytes)
{
    const QString path = m_dir.filePath(name);
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    f.write(bytes);
    return path;
}

MarkdownPreview *TestMarkdown::overlayOf(Editor *editor)
{
    return editor->findChild<MarkdownPreview *>("markdownRenderedOverlay", Qt::FindDirectChildrenOnly);
}

void TestMarkdown::initTestCase()
{
    QVERIFY(m_dir.isValid());
    Settings::instance().setRestoreSession(false);
}

void TestMarkdown::parsesAtxAndSetextHeadings()
{
    const auto h = MarkdownOutline::parseHeadings(
        "# One\ntext\n## Two ##\nSetext A\n===\npara\nSetext B\n---\n###### Six\n");
    QCOMPARE(h.size(), 5);
    QCOMPARE(h[0].level, 1); QCOMPARE(h[0].line, 1); QCOMPARE(h[0].title, QString("One"));
    QCOMPARE(h[1].level, 2); QCOMPARE(h[1].title, QString("Two"));
    QCOMPARE(h[2].level, 1); QCOMPARE(h[2].line, 4); QCOMPARE(h[2].title, QString("Setext A"));
    QCOMPARE(h[3].level, 2); QCOMPARE(h[3].line, 7);
    QCOMPARE(h[4].level, 6); QCOMPARE(h[4].line, 9);
}

void TestMarkdown::ignoresHeadingsInFencedCode()
{
    const auto h = MarkdownOutline::parseHeadings(
        "# Real\n```bash\n# comment, not a heading\n```\n~~~\n## also code\n~~~\n## After\n");
    QCOMPARE(h.size(), 2);
    QCOMPARE(h[1].title, QString("After"));
    QCOMPARE(h[1].line, 8);
}

void TestMarkdown::stripsInlineMarkupFromTitles()
{
    const auto h = MarkdownOutline::parseHeadings("## **Bold** and `code` [link](http://x)\n");
    QCOMPARE(h.size(), 1);
    QCOMPARE(h[0].title, QString("Bold and code link"));
}

void TestMarkdown::outlineNestsByLevel()
{
    MarkdownOutline outline;
    outline.setMarkdown("# A\n## A1\n### A1a\n## A2\n# B\n");
    QCOMPARE(outline.topLevelItemCount(), 2);
    QCOMPARE(outline.topLevelItem(0)->childCount(), 2);
    QCOMPARE(outline.topLevelItem(0)->child(0)->childCount(), 1);
    QCOMPARE(outline.topLevelItem(1)->text(0), QString("B"));
}

void TestMarkdown::previewRendersGithubTables()
{
    MarkdownPreview preview;
    preview.render("| a | b |\n|---|---|\n| 1 | 2 |\n\n- [x] done\n", QString());
    const QString html = preview.document()->toHtml();
    QVERIFY2(html.contains("<table"), "GitHub-dialect table must render as a table");
}

void TestMarkdown::previewBlocksRemoteImages()
{
    PreviewProbe preview;
    const QVariant remote = preview.loadResource(QTextDocument::ImageResource,
                                                 QUrl("http://example.invalid/x.png"));
    QVERIFY2(!remote.isValid(), "remote images must never be fetched");
    const QString notImage = writeFile("secret.txt", "not an image");
    QVERIFY(!preview.loadResource(QTextDocument::ImageResource, QUrl::fromLocalFile(notImage)).isValid());
}

void TestMarkdown::previewStylesInlineCode()
{
    // Run of 1.0.1 (2026-11-27): `code` rendered in the body font.
    MarkdownPreview preview;
    preview.render("plain `mono` plain\n", QString());
    const QTextBlock block = preview.document()->firstBlock();
    bool sawCode = false;
    for (auto it = block.begin(); !it.atEnd(); ++it) {
        const QTextFragment frag = it.fragment();
        if (frag.text().contains("mono")) {
            sawCode = true;
            const QString mono = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
            QCOMPARE(frag.charFormat().fontFamilies().toStringList().value(0), mono);
            QVERIFY(frag.charFormat().background().style() != Qt::NoBrush);
        } else if (frag.text().contains("plain")) {
            QVERIFY(frag.charFormat().fontFamilies().toStringList().isEmpty());
        }
    }
    QVERIFY(sawCode);
}

void TestMarkdown::previewScrollsToHeading()
{
    MarkdownPreview preview;
    preview.resize(400, 200);
    QString md;
    for (int i = 0; i < 6; ++i) {
        md += QString("# Heading %1\n\n").arg(i) + QString("filler line\n\n").repeated(30);
    }
    preview.show();
    preview.render(md, QString());
    QApplication::processEvents();
    preview.scrollToHeading(4);
    QVERIFY(preview.verticalScrollBar()->value() > 0);
    const int at4 = preview.verticalScrollBar()->value();
    preview.scrollToHeading(1);
    QVERIFY(preview.verticalScrollBar()->value() < at4);
}

void TestMarkdown::cycleSourceSideRenderedSource()
{
    MainWindow mw;
    mw.show();
    mw.openFile(writeFile("cycle.md", "# Title\n\nbody\n"));
    Editor *e = mw.currentEditor();
    auto *ctl = mw.findChild<MarkdownModeController *>();
    auto *dock = mw.findChild<QDockWidget *>("markdownPreviewDock");
    QVERIFY(e && ctl && dock);
    QCOMPARE(MarkdownModeController::modeOf(e), MarkdownModeController::Mode::Source);

    ctl->cycle(e);
    QCOMPARE(MarkdownModeController::modeOf(e), MarkdownModeController::Mode::SideBySide);
    QVERIFY(dock->isVisible());

    ctl->cycle(e);
    QCOMPARE(MarkdownModeController::modeOf(e), MarkdownModeController::Mode::Rendered);
    QVERIFY(!dock->isVisible());
    QVERIFY(overlayOf(e) && overlayOf(e)->isVisible());
    QCOMPARE(overlayOf(e)->geometry(), e->rect());
    QVERIFY(overlayOf(e)->toPlainText().contains("Title"));

    ctl->cycle(e);
    QCOMPARE(MarkdownModeController::modeOf(e), MarkdownModeController::Mode::Source);
    QVERIFY(!overlayOf(e)->isVisible());
    QCOMPARE(e->toPlainText(), QString("# Title\n\nbody\n")); // source untouched
}

void TestMarkdown::cycleIsNoOpForNonMarkdown()
{
    MainWindow mw;
    mw.show();
    mw.openFile(writeFile("plain.txt", "# not markdown\n"));
    auto *ctl = mw.findChild<MarkdownModeController *>();
    ctl->cycle(mw.currentEditor());
    QCOMPARE(MarkdownModeController::modeOf(mw.currentEditor()), MarkdownModeController::Mode::Source);
    QVERIFY(!mw.findChild<QDockWidget *>("markdownPreviewDock")->isVisible());
}

void TestMarkdown::outlineFollowsMarkdownTabsOnly()
{
    MainWindow mw;
    mw.show();
    auto *outlineDock = mw.findChild<QDockWidget *>("markdownOutlineDock");
    mw.openFile(writeFile("o.md", "# A\n## B\n"));
    QApplication::processEvents();
    QVERIFY(outlineDock->isVisible());
    auto *outline = qobject_cast<MarkdownOutline *>(outlineDock->widget());
    QCOMPARE(outline->topLevelItemCount(), 1);
    QCOMPARE(outline->topLevelItem(0)->childCount(), 1);

    mw.openFile(writeFile("o.txt", "plain\n"));
    QApplication::processEvents();
    QVERIFY(!outlineDock->isVisible());
}

void TestMarkdown::outlineClickGoesToHeadingLine()
{
    MainWindow mw;
    mw.show();
    mw.openFile(writeFile("nav.md", "# A\none\ntwo\n## B\nthree\n"));
    QApplication::processEvents();
    auto *outline = qobject_cast<MarkdownOutline *>(
        mw.findChild<QDockWidget *>("markdownOutlineDock")->widget());
    QTreeWidgetItem *b = outline->topLevelItem(0)->child(0);
    emit outline->itemClicked(b, 0);
    QCOMPARE(mw.currentEditor()->textCursor().blockNumber() + 1, 4);
}

void TestMarkdown::escapeLeavesRenderedMode()
{
    MainWindow mw;
    mw.show();
    mw.openFile(writeFile("esc.md", "# X\n"));
    Editor *e = mw.currentEditor();
    auto *ctl = mw.findChild<MarkdownModeController *>();
    ctl->setMode(e, MarkdownModeController::Mode::Rendered);
    QTest::keyClick(overlayOf(e), Qt::Key_Escape);
    QCOMPARE(MarkdownModeController::modeOf(e), MarkdownModeController::Mode::Source);
}

void TestMarkdown::livePreviewUpdatesWhileTyping()
{
    MainWindow mw;
    mw.show();
    mw.openFile(writeFile("live.md", "# Old\n"));
    Editor *e = mw.currentEditor();
    auto *ctl = mw.findChild<MarkdownModeController *>();
    ctl->setMode(e, MarkdownModeController::Mode::SideBySide);
    auto *side = qobject_cast<MarkdownPreview *>(mw.findChild<QDockWidget *>("markdownPreviewDock")->widget());
    e->moveCursor(QTextCursor::End);
    e->insertPlainText("\n## Added live\n");
    QTRY_VERIFY_WITH_TIMEOUT(side->toPlainText().contains("Added live"), 2000);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    isolateFromUserData();
    TestMarkdown tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_markdown.moc"

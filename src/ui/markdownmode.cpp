#include "markdownmode.h"

#include "core/document.h"
#include "editor.h"
#include "markdownoutline.h"
#include "markdownpreview.h"

#include <QAction>

#include <QDockWidget>
#include <QEvent>
#include <QFileInfo>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMainWindow>

namespace {
constexpr const char *kModeProperty = "markdownMode";
constexpr const char *kOverlayName = "markdownRenderedOverlay";
constexpr int kRenderDelayMs = 250; // live preview debounce while typing
}

MarkdownModeController::MarkdownModeController(QMainWindow *window)
    : QObject(window)
{
    m_sidePreview = createPreview(nullptr);
    m_dock = new QDockWidget(tr("Markdown Preview"), window);
    m_dock->setObjectName("markdownPreviewDock");
    m_dock->setWidget(m_sidePreview);
    m_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    window->addDockWidget(Qt::RightDockWidgetArea, m_dock);
    m_dock->hide();
    connect(m_dock, &QDockWidget::visibilityChanged,
            this, &MarkdownModeController::onDockVisibilityChanged);

    setupOutline(window);

    m_renderTimer.setSingleShot(true);
    m_renderTimer.setInterval(kRenderDelayMs);
    connect(&m_renderTimer, &QTimer::timeout, this, &MarkdownModeController::renderNow);
}

bool MarkdownModeController::isMarkdown(const Editor *editor)
{
    if (!editor || !editor->document()) {
        return false;
    }
    const Document *doc = editor->document();
    if (doc->language().compare("Markdown", Qt::CaseInsensitive) == 0) {
        return true;
    }
    const QString suffix = QFileInfo(doc->filePath()).suffix().toLower();
    return suffix == "md" || suffix == "markdown" || suffix == "mdown" || suffix == "mkd";
}

MarkdownModeController::Mode MarkdownModeController::modeOf(const Editor *editor)
{
    if (!editor) {
        return Mode::Source;
    }
    return static_cast<Mode>(editor->property(kModeProperty).toInt());
}

void MarkdownModeController::cycle(Editor *editor)
{
    if (!isMarkdown(editor)) {
        return;
    }
    const int next = (static_cast<int>(modeOf(editor)) + 1) % 3;
    setMode(editor, static_cast<Mode>(next));
}

void MarkdownModeController::setMode(Editor *editor, Mode mode)
{
    if (!editor) {
        return;
    }
    editor->setProperty(kModeProperty, static_cast<int>(mode));
    if (editor == m_active) {
        applyMode(editor);
    }
}

void MarkdownModeController::setActiveEditor(Editor *editor)
{
    disconnect(m_textConnection);
    disconnect(m_cursorConnection);
    m_active = editor;
    if (editor) {
        m_textConnection = connect(editor, &QPlainTextEdit::textChanged,
                                   this, &MarkdownModeController::scheduleRender);
        m_cursorConnection = connect(editor, &QPlainTextEdit::cursorPositionChanged, this, [this]() {
            if (m_active && m_outlineDock->isVisible()) {
                m_outline->setCurrentLine(m_active->textCursor().blockNumber() + 1);
            }
        });
    }
    updateOutlineVisibility();
    applyMode(editor);
}

void MarkdownModeController::applyMode(Editor *editor)
{
    m_applying = true;
    const Mode mode = isMarkdown(editor) ? modeOf(editor) : Mode::Source;
    m_dock->setVisible(mode == Mode::SideBySide);

    if (MarkdownPreview *overlay = overlayFor(editor, mode == Mode::Rendered)) {
        overlay->setGeometry(editor->rect());
        overlay->setVisible(mode == Mode::Rendered);
        if (mode == Mode::Rendered) {
            overlay->raise();
            overlay->setFocus();
        } else {
            editor->setFocus();
        }
    }
    m_applying = false;
    renderNow();
}

MarkdownPreview *MarkdownModeController::overlayFor(Editor *editor, bool create)
{
    if (!editor) {
        return nullptr;
    }
    auto *overlay = editor->findChild<MarkdownPreview *>(kOverlayName, Qt::FindDirectChildrenOnly);
    if (!overlay && create) {
        overlay = createPreview(editor); // owned by the editor: dies with the tab
        overlay->setObjectName(kOverlayName);
        editor->installEventFilter(this);
        overlay->installEventFilter(this);
    }
    return overlay;
}

MarkdownPreview *MarkdownModeController::createPreview(QWidget *parent)
{
    auto *preview = new MarkdownPreview(parent);
    connect(preview, &MarkdownPreview::openFileRequested,
            this, &MarkdownModeController::openFileRequested);
    return preview;
}

void MarkdownModeController::scheduleRender()
{
    if (m_active && isMarkdown(m_active)) {
        m_renderTimer.start();
    }
}

void MarkdownModeController::renderNow()
{
    Editor *editor = m_active;
    if (!editor || !isMarkdown(editor)) {
        return;
    }
    const QString baseDir = editor->document()->filePath().isEmpty()
        ? QString() : QFileInfo(editor->document()->filePath()).absolutePath();
    const QString text = editor->toPlainText();
    if (m_outlineDock->isVisible()) {
        m_outline->setMarkdown(text);
        m_outline->setCurrentLine(editor->textCursor().blockNumber() + 1);
    }
    if (modeOf(editor) == Mode::SideBySide) {
        m_sidePreview->render(text, baseDir);
    } else if (modeOf(editor) == Mode::Rendered) {
        if (MarkdownPreview *overlay = overlayFor(editor, false)) {
            overlay->render(text, baseDir);
        }
    }
}

void MarkdownModeController::onDockVisibilityChanged(bool visible)
{
    // Closing the dock with its X button means "back to source" for this tab.
    if (!visible && !m_applying && m_active && modeOf(m_active) == Mode::SideBySide) {
        m_active->setProperty(kModeProperty, static_cast<int>(Mode::Source));
    }
}

void MarkdownModeController::setupOutline(QMainWindow *window)
{
    m_outline = new MarkdownOutline();
    m_outlineDock = new QDockWidget(tr("Outline"), window);
    m_outlineDock->setObjectName("markdownOutlineDock");
    m_outlineDock->setWidget(m_outline);
    m_outlineDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    window->addDockWidget(Qt::LeftDockWidgetArea, m_outlineDock);
    m_outlineDock->hide();
    connect(m_outline, &MarkdownOutline::headingActivated,
            this, &MarkdownModeController::onHeadingActivated);

    m_outlineAction = new QAction(tr("Markdown &Outline"), this);
    m_outlineAction->setCheckable(true);
    m_outlineAction->setChecked(true); // shown automatically for Markdown tabs
    m_outlineAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H));
    connect(m_outlineAction, &QAction::toggled, this, [this]() {
        updateOutlineVisibility();
        renderNow();
    });
    // Closing the panel with its X button turns the outline off.
    connect(m_outlineDock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (!visible && !m_applying && isMarkdown(m_active)) {
            m_outlineAction->setChecked(false);
        }
    });
}

void MarkdownModeController::updateOutlineVisibility()
{
    m_applying = true;
    m_outlineDock->setVisible(m_outlineAction->isChecked() && isMarkdown(m_active));
    m_applying = false;
}

void MarkdownModeController::onHeadingActivated(int line, int headingIndex)
{
    if (!m_active) {
        return;
    }
    m_active->goToLine(line);
    const Mode mode = modeOf(m_active);
    if (mode == Mode::SideBySide) {
        m_sidePreview->scrollToHeading(headingIndex);
    } else if (mode == Mode::Rendered) {
        if (MarkdownPreview *overlay = overlayFor(m_active, false)) {
            overlay->scrollToHeading(headingIndex);
        }
    } else {
        m_active->setFocus();
    }
}

bool MarkdownModeController::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Resize) {
        if (auto *editor = qobject_cast<Editor *>(watched)) {
            if (MarkdownPreview *overlay = overlayFor(editor, false)) {
                overlay->setGeometry(editor->rect());
            }
        }
    } else if (event->type() == QEvent::KeyPress
               && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape) {
        auto *overlay = qobject_cast<MarkdownPreview *>(watched);
        if (overlay && overlay->objectName() == kOverlayName) {
            setMode(qobject_cast<Editor *>(overlay->parentWidget()), Mode::Source);
            return true; // Esc in the rendered view returns to the source
        }
    }
    return QObject::eventFilter(watched, event);
}

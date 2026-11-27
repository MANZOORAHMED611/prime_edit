#ifndef MARKDOWNMODE_H
#define MARKDOWNMODE_H

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QTimer>

class Editor;
class MarkdownOutline;
class MarkdownPreview;
class QAction;
class QDockWidget;
class QMainWindow;

// Per-tab Markdown view modes, cycled with View > Markdown Preview:
//   Source     - plain text with all Markdown symbols (the editor itself)
//   SideBySide - editor plus a live preview in a right dock that follows the tab
//   Rendered   - a preview laid over the editor, covering it entirely
// The mode lives on the Editor (dynamic property), so each tab remembers its
// own and no tab/editor structure changes: tabs still hold plain Editors.
class MarkdownModeController : public QObject
{
    Q_OBJECT

public:
    enum class Mode { Source = 0, SideBySide = 1, Rendered = 2 };

    explicit MarkdownModeController(QMainWindow *window);

    QDockWidget *dock() const { return m_dock; }
    QDockWidget *outlineDock() const { return m_outlineDock; }
    // Checkable View-menu action: show the heading outline for Markdown tabs.
    QAction *outlineAction() const { return m_outlineAction; }
    static bool isMarkdown(const Editor *editor);
    static Mode modeOf(const Editor *editor);

    // Advances Source -> SideBySide -> Rendered -> Source. No-op for non-Markdown.
    void cycle(Editor *editor);
    void setMode(Editor *editor, Mode mode);
    void setActiveEditor(Editor *editor);

signals:
    void openFileRequested(const QString &path);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    MarkdownPreview *overlayFor(Editor *editor, bool create);
    MarkdownPreview *createPreview(QWidget *parent);
    void applyMode(Editor *editor);
    void scheduleRender();
    void renderNow();
    void onDockVisibilityChanged(bool visible);
    void setupOutline(QMainWindow *window);
    void updateOutlineVisibility();
    void onHeadingActivated(int line, int headingIndex);

    QDockWidget *m_dock = nullptr;
    MarkdownPreview *m_sidePreview = nullptr;
    QPointer<Editor> m_active;
    QDockWidget *m_outlineDock = nullptr;
    MarkdownOutline *m_outline = nullptr;
    QAction *m_outlineAction = nullptr;
    QMetaObject::Connection m_textConnection;
    QMetaObject::Connection m_cursorConnection;
    QTimer m_renderTimer;
    bool m_applying = false;
};

#endif // MARKDOWNMODE_H

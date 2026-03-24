#ifndef MARKDOWNPREVIEW_H
#define MARKDOWNPREVIEW_H

#include <QWidget>
#include <QTextBrowser>
#include <QTimer>
#include <QString>

class MarkdownPreview : public QWidget
{
    Q_OBJECT

public:
    explicit MarkdownPreview(QWidget *parent = nullptr);
    ~MarkdownPreview() override = default;

    void setMarkdown(const QString &markdown);
    void updatePreview();

    void setAutoUpdate(bool enabled);
    bool autoUpdate() const { return m_autoUpdate; }

    void setUpdateDelay(int ms);
    int updateDelay() const { return m_updateDelay; }

public slots:
    void refresh();
    void zoomIn();
    void zoomOut();
    void resetZoom();

private:
    void setupUI();
    QString markdownToHtml(const QString &markdown);
    QString generateCSS();

    QTextBrowser *m_browser;
    QTimer *m_updateTimer;
    QString m_markdown;
    bool m_autoUpdate;
    int m_updateDelay;
    qreal m_zoomFactor;
};

#endif // MARKDOWNPREVIEW_H

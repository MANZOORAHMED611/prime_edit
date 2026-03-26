#include "statusbar.h"
#include "editor.h"
#include "mainwindow.h"
#include "theme.h"
#include "core/document.h"

StatusBarWidget::StatusBarWidget(MainWindow *parent)
    : QStatusBar(parent)
    , m_mainWindow(parent)
{
    m_docTypeLabel = new QLabel(this);
    m_lengthLabel = new QLabel(this);
    m_positionLabel = new QLabel(this);
    m_eolButton = new QPushButton(this);
    m_encodingButton = new QPushButton(this);
    m_insertModeLabel = new QLabel(this);

    // Frame style for labels
    m_docTypeLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_lengthLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_positionLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_insertModeLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    // Flat style for buttons
    m_eolButton->setFlat(true);
    m_encodingButton->setFlat(true);

    // Layout: docType stretches, rest are permanent
    addWidget(m_docTypeLabel, 1);
    addPermanentWidget(m_lengthLabel);
    addPermanentWidget(m_positionLabel);
    addPermanentWidget(m_eolButton);
    addPermanentWidget(m_encodingButton);
    addPermanentWidget(m_insertModeLabel);

    // Set default values
    setDocType("Plain Text");
    setLength(0, 1);
    setPosition(1, 1, 0, 0);
    setLineEnding("LF");
    setEncoding("UTF-8");
    setInsertMode(true);

    // Setup menus
    setupEolMenu();
    setupEncodingMenu();

    // Apply theme-aware styles (initial + on theme change)
    auto applyThemeStyle = [this]() {
        Theme theme = ThemeManager::instance().currentTheme();
        QString statusStyle = QString(
            "QStatusBar { border-top: 1px solid %1; background: %2; } "
            "QLabel { padding: 2px 6px; color: %3; font-size: 11px; }")
            .arg(theme.borderColor.name(),
                 theme.statusBarBackground.name(),
                 theme.statusBarForeground.name());
        setStyleSheet(statusStyle);

        QString btnStyle = QString(
            "QPushButton { border: 1px solid %1; padding: 2px 8px; "
            "background: transparent; color: %2; } "
            "QPushButton:hover { background: %3; }")
            .arg(theme.borderColor.name(),
                 theme.statusBarForeground.name(),
                 theme.accentPrimary.name());
        m_eolButton->setStyleSheet(btnStyle);
        m_encodingButton->setStyleSheet(btnStyle);
    };
    applyThemeStyle();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, applyThemeStyle);
}

void StatusBarWidget::setupEolMenu()
{
    m_eolMenu = new QMenu(this);

    QAction *windowsAction = m_eolMenu->addAction("Windows (CR LF)");
    QAction *unixAction = m_eolMenu->addAction("Unix (LF)");
    QAction *macAction = m_eolMenu->addAction("Mac (CR)");

    connect(windowsAction, &QAction::triggered,
            m_mainWindow, &MainWindow::convertToWindows);
    connect(unixAction, &QAction::triggered,
            m_mainWindow, &MainWindow::convertToUnix);
    connect(macAction, &QAction::triggered,
            m_mainWindow, &MainWindow::convertToMac);

    m_eolButton->setMenu(m_eolMenu);
}

void StatusBarWidget::setupEncodingMenu()
{
    m_encodingMenu = new QMenu(this);

    QAction *utf8Action = m_encodingMenu->addAction("UTF-8");
    QAction *utf8BomAction = m_encodingMenu->addAction("UTF-8-BOM");
    QAction *ansiAction = m_encodingMenu->addAction("ANSI");
    QAction *ucs2BeAction = m_encodingMenu->addAction("UCS-2 Big Endian");
    QAction *ucs2LeAction = m_encodingMenu->addAction("UCS-2 Little Endian");

    connect(utf8Action, &QAction::triggered,
            m_mainWindow, &MainWindow::convertToUTF8);
    connect(utf8BomAction, &QAction::triggered,
            m_mainWindow, &MainWindow::convertToUTF8BOM);
    connect(ansiAction, &QAction::triggered,
            m_mainWindow, &MainWindow::convertToANSI);
    connect(ucs2BeAction, &QAction::triggered,
            m_mainWindow, &MainWindow::convertToUCS2BE);
    connect(ucs2LeAction, &QAction::triggered,
            m_mainWindow, &MainWindow::convertToUCS2LE);

    m_encodingButton->setMenu(m_encodingMenu);
}

void StatusBarWidget::updateFromEditor(Editor *editor)
{
    if (!editor) {
        return;
    }

    setPosition(editor->currentLine(), editor->currentColumn(),
                editor->selectedText().length(),
                editor->textCursor().position());

    Document *doc = editor->document();
    if (doc) {
        setEncoding(doc->encoding());

        QString eolStr;
        switch (doc->lineEnding()) {
        case Document::Unix:
            eolStr = "Unix (LF)";
            break;
        case Document::Windows:
            eolStr = "Windows (CR LF)";
            break;
        case Document::ClassicMac:
            eolStr = "Mac (CR)";
            break;
        }
        setLineEnding(eolStr);

        QString lang = doc->language();
        setLanguage(lang.isEmpty() ? "Plain Text" : lang);

        setLength(doc->text().length(), doc->text().count('\n') + 1);
    }
}

void StatusBarWidget::setDocType(const QString &type)
{
    m_docTypeLabel->setText(type);
}

void StatusBarWidget::setLength(qint64 length, int lines)
{
    m_lengthLabel->setText(
        QString("Length : %1    Lines : %2").arg(length).arg(lines));
}

void StatusBarWidget::setPosition(int line, int column,
                                  int selCount, qint64 pos)
{
    m_positionLabel->setText(
        QString("Ln : %1    Col : %2    Sel : %3    Pos : %4")
            .arg(line).arg(column).arg(selCount).arg(pos));
}

void StatusBarWidget::setEncoding(const QString &encoding)
{
    m_encodingButton->setText(encoding);
}

void StatusBarWidget::setLineEnding(const QString &lineEnding)
{
    m_eolButton->setText(lineEnding);
}

void StatusBarWidget::setLanguage(const QString &language)
{
    m_docTypeLabel->setText(language);
}

void StatusBarWidget::setInsertMode(bool insert)
{
    m_insertModeLabel->setText(insert ? "INS" : "OVR");
}

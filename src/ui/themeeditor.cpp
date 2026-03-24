#include "themeeditor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QColorDialog>
#include <QPainter>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QSplitter>
#include <QLabel>
#include <QJsonDocument>

// ColorButton implementation
ColorButton::ColorButton(QWidget *parent)
    : QPushButton(parent)
    , m_color(Qt::white)
{
    setMinimumSize(80, 30);
    setMaximumHeight(30);
}

void ColorButton::setColor(const QColor &color)
{
    if (m_color != color) {
        m_color = color;
        update();
        emit colorChanged(color);
    }
}

void ColorButton::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw color swatch
    QRect colorRect = rect().adjusted(4, 4, -4, -4);
    painter.fillRect(colorRect, m_color);
    painter.setPen(Qt::black);
    painter.drawRect(colorRect);

    // Draw color name
    painter.setPen(Qt::white);
    painter.drawText(colorRect, Qt::AlignCenter, m_color.name());
}

void ColorButton::mouseReleaseEvent(QMouseEvent *event)
{
    QPushButton::mouseReleaseEvent(event);

    QColor newColor = QColorDialog::getColor(m_color, this, "Select Color");
    if (newColor.isValid()) {
        setColor(newColor);
    }
}

// ThemeEditorDialog implementation
ThemeEditorDialog::ThemeEditorDialog(QWidget *parent)
    : QDialog(parent)
    , m_theme(Theme::defaultDark())
{
    setWindowTitle(tr("Theme Editor"));
    resize(1200, 800);
    setupUI();
    updatePreview();
}

void ThemeEditorDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Create splitter for editor and preview
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

    // Left side: Editor controls
    QWidget *editorWidget = new QWidget;
    QVBoxLayout *editorLayout = new QVBoxLayout(editorWidget);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget *scrollContent = new QWidget;
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);

    createMetadataSection(scrollContent);
    createEditorColorsSection(scrollContent);
    createSyntaxColorsSection(scrollContent);
    createUIColorsSection(scrollContent);
    createAccentColorsSection(scrollContent);
    createDiagnosticColorsSection(scrollContent);

    scrollLayout->addStretch();
    scrollArea->setWidget(scrollContent);
    editorLayout->addWidget(scrollArea);

    // Right side: Preview
    QWidget *previewWidget = new QWidget;
    QVBoxLayout *previewLayout = new QVBoxLayout(previewWidget);
    createPreviewSection(previewWidget);

    splitter->addWidget(editorWidget);
    splitter->addWidget(previewWidget);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);

    mainLayout->addWidget(splitter);

    // Bottom: Buttons
    createButtonSection(this);
}

void ThemeEditorDialog::createMetadataSection(QWidget *container)
{
    QGroupBox *group = new QGroupBox(tr("Theme Information"), container);
    QFormLayout *layout = new QFormLayout(group);

    m_nameEdit = new QLineEdit(m_theme.name);
    m_authorEdit = new QLineEdit(m_theme.author);
    m_isDarkCheckbox = new QCheckBox(tr("Dark Theme"));
    m_isDarkCheckbox->setChecked(m_theme.isDark);

    layout->addRow(tr("Name:"), m_nameEdit);
    layout->addRow(tr("Author:"), m_authorEdit);
    layout->addRow(tr("Type:"), m_isDarkCheckbox);

    connect(m_nameEdit, &QLineEdit::textChanged, this, &ThemeEditorDialog::onMetadataChanged);
    connect(m_authorEdit, &QLineEdit::textChanged, this, &ThemeEditorDialog::onMetadataChanged);
    connect(m_isDarkCheckbox, &QCheckBox::toggled, this, &ThemeEditorDialog::onMetadataChanged);

    container->layout()->addWidget(group);
}

void ThemeEditorDialog::createEditorColorsSection(QWidget *container)
{
    QGroupBox *group = new QGroupBox(tr("Editor Colors"), container);
    QGridLayout *layout = new QGridLayout(group);

    int row = 0;
    m_backgroundBtn = createColorButton(tr("Background"), m_theme.background, group);
    layout->addWidget(new QLabel(tr("Background:")), row, 0);
    layout->addWidget(m_backgroundBtn, row++, 1);
    connectColorButton(m_backgroundBtn, &Theme::background);

    m_foregroundBtn = createColorButton(tr("Foreground"), m_theme.foreground, group);
    layout->addWidget(new QLabel(tr("Foreground:")), row, 0);
    layout->addWidget(m_foregroundBtn, row++, 1);
    connectColorButton(m_foregroundBtn, &Theme::foreground);

    m_selectionBgBtn = createColorButton(tr("Selection Background"), m_theme.selectionBackground, group);
    layout->addWidget(new QLabel(tr("Selection Background:")), row, 0);
    layout->addWidget(m_selectionBgBtn, row++, 1);
    connectColorButton(m_selectionBgBtn, &Theme::selectionBackground);

    m_selectionFgBtn = createColorButton(tr("Selection Foreground"), m_theme.selectionForeground, group);
    layout->addWidget(new QLabel(tr("Selection Foreground:")), row, 0);
    layout->addWidget(m_selectionFgBtn, row++, 1);
    connectColorButton(m_selectionFgBtn, &Theme::selectionForeground);

    m_currentLineBgBtn = createColorButton(tr("Current Line"), m_theme.currentLineBackground, group);
    layout->addWidget(new QLabel(tr("Current Line:")), row, 0);
    layout->addWidget(m_currentLineBgBtn, row++, 1);
    connectColorButton(m_currentLineBgBtn, &Theme::currentLineBackground);

    m_lineNumberFgBtn = createColorButton(tr("Line Number Foreground"), m_theme.lineNumberForeground, group);
    layout->addWidget(new QLabel(tr("Line Number Foreground:")), row, 0);
    layout->addWidget(m_lineNumberFgBtn, row++, 1);
    connectColorButton(m_lineNumberFgBtn, &Theme::lineNumberForeground);

    m_lineNumberBgBtn = createColorButton(tr("Line Number Background"), m_theme.lineNumberBackground, group);
    layout->addWidget(new QLabel(tr("Line Number Background:")), row, 0);
    layout->addWidget(m_lineNumberBgBtn, row++, 1);
    connectColorButton(m_lineNumberBgBtn, &Theme::lineNumberBackground);

    container->layout()->addWidget(group);
}

void ThemeEditorDialog::createSyntaxColorsSection(QWidget *container)
{
    QGroupBox *group = new QGroupBox(tr("Syntax Colors"), container);
    QGridLayout *layout = new QGridLayout(group);

    int row = 0;
    m_keywordBtn = createColorButton(tr("Keyword"), m_theme.keyword, group);
    layout->addWidget(new QLabel(tr("Keyword:")), row, 0);
    layout->addWidget(m_keywordBtn, row++, 1);
    connectColorButton(m_keywordBtn, &Theme::keyword);

    m_stringBtn = createColorButton(tr("String"), m_theme.string, group);
    layout->addWidget(new QLabel(tr("String:")), row, 0);
    layout->addWidget(m_stringBtn, row++, 1);
    connectColorButton(m_stringBtn, &Theme::string);

    m_commentBtn = createColorButton(tr("Comment"), m_theme.comment, group);
    layout->addWidget(new QLabel(tr("Comment:")), row, 0);
    layout->addWidget(m_commentBtn, row++, 1);
    connectColorButton(m_commentBtn, &Theme::comment);

    m_numberBtn = createColorButton(tr("Number"), m_theme.number, group);
    layout->addWidget(new QLabel(tr("Number:")), row, 0);
    layout->addWidget(m_numberBtn, row++, 1);
    connectColorButton(m_numberBtn, &Theme::number);

    m_functionBtn = createColorButton(tr("Function"), m_theme.function, group);
    layout->addWidget(new QLabel(tr("Function:")), row, 0);
    layout->addWidget(m_functionBtn, row++, 1);
    connectColorButton(m_functionBtn, &Theme::function);

    m_typeBtn = createColorButton(tr("Type"), m_theme.type, group);
    layout->addWidget(new QLabel(tr("Type:")), row, 0);
    layout->addWidget(m_typeBtn, row++, 1);
    connectColorButton(m_typeBtn, &Theme::type);

    m_variableBtn = createColorButton(tr("Variable"), m_theme.variable, group);
    layout->addWidget(new QLabel(tr("Variable:")), row, 0);
    layout->addWidget(m_variableBtn, row++, 1);
    connectColorButton(m_variableBtn, &Theme::variable);

    m_operatorBtn = createColorButton(tr("Operator"), m_theme.operator_, group);
    layout->addWidget(new QLabel(tr("Operator:")), row, 0);
    layout->addWidget(m_operatorBtn, row++, 1);
    connectColorButton(m_operatorBtn, &Theme::operator_);

    container->layout()->addWidget(group);
}

void ThemeEditorDialog::createUIColorsSection(QWidget *container)
{
    QGroupBox *group = new QGroupBox(tr("UI Colors"), container);
    QGridLayout *layout = new QGridLayout(group);

    int row = 0;
    m_menuBgBtn = createColorButton(tr("Menu Background"), m_theme.menuBackground, group);
    layout->addWidget(new QLabel(tr("Menu Background:")), row, 0);
    layout->addWidget(m_menuBgBtn, row++, 1);
    connectColorButton(m_menuBgBtn, &Theme::menuBackground);

    m_menuFgBtn = createColorButton(tr("Menu Foreground"), m_theme.menuForeground, group);
    layout->addWidget(new QLabel(tr("Menu Foreground:")), row, 0);
    layout->addWidget(m_menuFgBtn, row++, 1);
    connectColorButton(m_menuFgBtn, &Theme::menuForeground);

    m_toolbarBgBtn = createColorButton(tr("Toolbar Background"), m_theme.toolbarBackground, group);
    layout->addWidget(new QLabel(tr("Toolbar Background:")), row, 0);
    layout->addWidget(m_toolbarBgBtn, row++, 1);
    connectColorButton(m_toolbarBgBtn, &Theme::toolbarBackground);

    m_statusBarBgBtn = createColorButton(tr("Status Bar Background"), m_theme.statusBarBackground, group);
    layout->addWidget(new QLabel(tr("Status Bar Background:")), row, 0);
    layout->addWidget(m_statusBarBgBtn, row++, 1);
    connectColorButton(m_statusBarBgBtn, &Theme::statusBarBackground);

    m_statusBarFgBtn = createColorButton(tr("Status Bar Foreground"), m_theme.statusBarForeground, group);
    layout->addWidget(new QLabel(tr("Status Bar Foreground:")), row, 0);
    layout->addWidget(m_statusBarFgBtn, row++, 1);
    connectColorButton(m_statusBarFgBtn, &Theme::statusBarForeground);

    m_tabActiveBgBtn = createColorButton(tr("Active Tab Background"), m_theme.tabActiveBackground, group);
    layout->addWidget(new QLabel(tr("Active Tab Background:")), row, 0);
    layout->addWidget(m_tabActiveBgBtn, row++, 1);
    connectColorButton(m_tabActiveBgBtn, &Theme::tabActiveBackground);

    m_tabInactiveBgBtn = createColorButton(tr("Inactive Tab Background"), m_theme.tabInactiveBackground, group);
    layout->addWidget(new QLabel(tr("Inactive Tab Background:")), row, 0);
    layout->addWidget(m_tabInactiveBgBtn, row++, 1);
    connectColorButton(m_tabInactiveBgBtn, &Theme::tabInactiveBackground);

    m_tabFgBtn = createColorButton(tr("Tab Foreground"), m_theme.tabForeground, group);
    layout->addWidget(new QLabel(tr("Tab Foreground:")), row, 0);
    layout->addWidget(m_tabFgBtn, row++, 1);
    connectColorButton(m_tabFgBtn, &Theme::tabForeground);

    container->layout()->addWidget(group);
}

void ThemeEditorDialog::createAccentColorsSection(QWidget *container)
{
    QGroupBox *group = new QGroupBox(tr("Accent Colors"), container);
    QGridLayout *layout = new QGridLayout(group);

    int row = 0;
    m_accentPrimaryBtn = createColorButton(tr("Primary Accent"), m_theme.accentPrimary, group);
    layout->addWidget(new QLabel(tr("Primary Accent:")), row, 0);
    layout->addWidget(m_accentPrimaryBtn, row++, 1);
    connectColorButton(m_accentPrimaryBtn, &Theme::accentPrimary);

    m_accentSecondaryBtn = createColorButton(tr("Secondary Accent"), m_theme.accentSecondary, group);
    layout->addWidget(new QLabel(tr("Secondary Accent:")), row, 0);
    layout->addWidget(m_accentSecondaryBtn, row++, 1);
    connectColorButton(m_accentSecondaryBtn, &Theme::accentSecondary);

    m_borderColorBtn = createColorButton(tr("Border Color"), m_theme.borderColor, group);
    layout->addWidget(new QLabel(tr("Border Color:")), row, 0);
    layout->addWidget(m_borderColorBtn, row++, 1);
    connectColorButton(m_borderColorBtn, &Theme::borderColor);

    container->layout()->addWidget(group);
}

void ThemeEditorDialog::createDiagnosticColorsSection(QWidget *container)
{
    QGroupBox *group = new QGroupBox(tr("Diagnostic Colors"), container);
    QGridLayout *layout = new QGridLayout(group);

    int row = 0;
    m_errorFgBtn = createColorButton(tr("Error"), m_theme.errorForeground, group);
    layout->addWidget(new QLabel(tr("Error:")), row, 0);
    layout->addWidget(m_errorFgBtn, row++, 1);
    connectColorButton(m_errorFgBtn, &Theme::errorForeground);

    m_warningFgBtn = createColorButton(tr("Warning"), m_theme.warningForeground, group);
    layout->addWidget(new QLabel(tr("Warning:")), row, 0);
    layout->addWidget(m_warningFgBtn, row++, 1);
    connectColorButton(m_warningFgBtn, &Theme::warningForeground);

    m_infoFgBtn = createColorButton(tr("Info"), m_theme.infoForeground, group);
    layout->addWidget(new QLabel(tr("Info:")), row, 0);
    layout->addWidget(m_infoFgBtn, row++, 1);
    connectColorButton(m_infoFgBtn, &Theme::infoForeground);

    m_hintFgBtn = createColorButton(tr("Hint"), m_theme.hintForeground, group);
    layout->addWidget(new QLabel(tr("Hint:")), row, 0);
    layout->addWidget(m_hintFgBtn, row++, 1);
    connectColorButton(m_hintFgBtn, &Theme::hintForeground);

    container->layout()->addWidget(group);
}

void ThemeEditorDialog::createPreviewSection(QWidget *container)
{
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(container->layout());
    if (!layout) {
        layout = new QVBoxLayout(container);
    }

    QLabel *label = new QLabel(tr("Live Preview"), container);
    QFont font = label->font();
    font.setBold(true);
    font.setPointSize(font.pointSize() + 2);
    label->setFont(font);
    layout->addWidget(label);

    m_previewContainer = new QWidget(container);
    QVBoxLayout *previewLayout = new QVBoxLayout(m_previewContainer);
    previewLayout->setContentsMargins(0, 0, 0, 0);

    m_previewEdit = new QTextEdit(m_previewContainer);
    m_previewEdit->setReadOnly(true);
    m_previewEdit->setPlainText(
        "// Sample code preview\n"
        "#include <iostream>\n"
        "#include <string>\n"
        "\n"
        "// This is a comment\n"
        "class Example {\n"
        "public:\n"
        "    int value = 42;\n"
        "    std::string name = \"Hello, World!\";\n"
        "    \n"
        "    void display() {\n"
        "        std::cout << \"Value: \" << value << std::endl;\n"
        "        std::cout << \"Name: \" << name << std::endl;\n"
        "    }\n"
        "    \n"
        "    int calculate(int x, int y) {\n"
        "        return x + y * 2;\n"
        "    }\n"
        "};\n"
        "\n"
        "int main() {\n"
        "    Example obj;\n"
        "    obj.display();\n"
        "    return 0;\n"
        "}\n"
    );

    previewLayout->addWidget(m_previewEdit);
    layout->addWidget(m_previewContainer);
}

void ThemeEditorDialog::createButtonSection(QWidget *container)
{
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(container->layout());

    QHBoxLayout *buttonLayout = new QHBoxLayout;

    QPushButton *saveBtn = new QPushButton(tr("Save Theme"));
    QPushButton *loadBtn = new QPushButton(tr("Load Theme"));
    QPushButton *exportBtn = new QPushButton(tr("Export..."));
    QPushButton *importBtn = new QPushButton(tr("Import..."));
    QPushButton *resetBtn = new QPushButton(tr("Reset to Defaults"));
    QPushButton *closeBtn = new QPushButton(tr("Close"));

    connect(saveBtn, &QPushButton::clicked, this, &ThemeEditorDialog::saveTheme);
    connect(loadBtn, &QPushButton::clicked, this, &ThemeEditorDialog::loadTheme);
    connect(exportBtn, &QPushButton::clicked, this, &ThemeEditorDialog::exportTheme);
    connect(importBtn, &QPushButton::clicked, this, &ThemeEditorDialog::importTheme);
    connect(resetBtn, &QPushButton::clicked, this, &ThemeEditorDialog::resetToDefaults);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(loadBtn);
    buttonLayout->addWidget(exportBtn);
    buttonLayout->addWidget(importBtn);
    buttonLayout->addWidget(resetBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);

    mainLayout->addLayout(buttonLayout);
}

ColorButton* ThemeEditorDialog::createColorButton(const QString &label, const QColor &color, QWidget *parent)
{
    ColorButton *button = new ColorButton(parent);
    button->setColor(color);
    return button;
}

void ThemeEditorDialog::connectColorButton(ColorButton *button, QColor Theme::*member)
{
    connect(button, &ColorButton::colorChanged, this, [this, member](const QColor &color) {
        m_theme.*member = color;
        updatePreview();
    });
}

void ThemeEditorDialog::setTheme(const Theme &theme)
{
    m_theme = theme;

    // Update metadata
    m_nameEdit->setText(theme.name);
    m_authorEdit->setText(theme.author);
    m_isDarkCheckbox->setChecked(theme.isDark);

    // Update editor colors
    m_backgroundBtn->setColor(theme.background);
    m_foregroundBtn->setColor(theme.foreground);
    m_selectionBgBtn->setColor(theme.selectionBackground);
    m_selectionFgBtn->setColor(theme.selectionForeground);
    m_currentLineBgBtn->setColor(theme.currentLineBackground);
    m_lineNumberFgBtn->setColor(theme.lineNumberForeground);
    m_lineNumberBgBtn->setColor(theme.lineNumberBackground);

    // Update syntax colors
    m_keywordBtn->setColor(theme.keyword);
    m_stringBtn->setColor(theme.string);
    m_commentBtn->setColor(theme.comment);
    m_numberBtn->setColor(theme.number);
    m_functionBtn->setColor(theme.function);
    m_typeBtn->setColor(theme.type);
    m_variableBtn->setColor(theme.variable);
    m_operatorBtn->setColor(theme.operator_);

    // Update UI colors
    m_menuBgBtn->setColor(theme.menuBackground);
    m_menuFgBtn->setColor(theme.menuForeground);
    m_toolbarBgBtn->setColor(theme.toolbarBackground);
    m_statusBarBgBtn->setColor(theme.statusBarBackground);
    m_statusBarFgBtn->setColor(theme.statusBarForeground);
    m_tabActiveBgBtn->setColor(theme.tabActiveBackground);
    m_tabInactiveBgBtn->setColor(theme.tabInactiveBackground);
    m_tabFgBtn->setColor(theme.tabForeground);

    // Update accent colors
    m_accentPrimaryBtn->setColor(theme.accentPrimary);
    m_accentSecondaryBtn->setColor(theme.accentSecondary);
    m_borderColorBtn->setColor(theme.borderColor);

    // Update diagnostic colors
    m_errorFgBtn->setColor(theme.errorForeground);
    m_warningFgBtn->setColor(theme.warningForeground);
    m_infoFgBtn->setColor(theme.infoForeground);
    m_hintFgBtn->setColor(theme.hintForeground);

    updatePreview();
}

void ThemeEditorDialog::onColorChanged()
{
    updatePreview();
}

void ThemeEditorDialog::onMetadataChanged()
{
    m_theme.name = m_nameEdit->text();
    m_theme.author = m_authorEdit->text();
    m_theme.isDark = m_isDarkCheckbox->isChecked();
}

void ThemeEditorDialog::updatePreview()
{
    // Apply theme colors to preview widget
    QString style = QString(
        "QTextEdit {"
        "    background-color: %1;"
        "    color: %2;"
        "    selection-background-color: %3;"
        "    selection-color: %4;"
        "    border: 1px solid %5;"
        "}"
    ).arg(m_theme.background.name())
     .arg(m_theme.foreground.name())
     .arg(m_theme.selectionBackground.name())
     .arg(m_theme.selectionForeground.name())
     .arg(m_theme.borderColor.name());

    m_previewEdit->setStyleSheet(style);

    // Update container background
    QString containerStyle = QString(
        "QWidget {"
        "    background-color: %1;"
        "}"
    ).arg(m_theme.background.name());

    m_previewContainer->setStyleSheet(containerStyle);
}

void ThemeEditorDialog::saveTheme()
{
    onMetadataChanged();

    if (m_theme.name.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Theme"), tr("Please enter a theme name."));
        return;
    }

    ThemeManager::instance().saveTheme(m_theme);
    QMessageBox::information(this, tr("Theme Saved"), tr("Theme '%1' has been saved successfully.").arg(m_theme.name));
}

void ThemeEditorDialog::loadTheme()
{
    QVector<Theme> themes = ThemeManager::instance().themes();
    QStringList themeNames;
    for (const Theme &theme : themes) {
        themeNames << theme.name;
    }

    bool ok;
    QString themeName = QInputDialog::getItem(this, tr("Load Theme"), tr("Select a theme:"), themeNames, 0, false, &ok);

    if (ok && !themeName.isEmpty()) {
        Theme theme = ThemeManager::instance().theme(themeName);
        setTheme(theme);
    }
}

void ThemeEditorDialog::exportTheme()
{
    onMetadataChanged();

    QString filePath = QFileDialog::getSaveFileName(this, tr("Export Theme"),
                                                     m_theme.name + ".json",
                                                     tr("JSON Files (*.json)"));
    if (filePath.isEmpty()) {
        return;
    }

    if (ThemeManager::instance().exportTheme(m_theme.name, filePath)) {
        QMessageBox::information(this, tr("Export Successful"), tr("Theme exported to %1").arg(filePath));
    } else {
        QMessageBox::warning(this, tr("Export Failed"), tr("Failed to export theme to %1").arg(filePath));
    }
}

void ThemeEditorDialog::importTheme()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Import Theme"),
                                                     QString(),
                                                     tr("JSON Files (*.json)"));
    if (filePath.isEmpty()) {
        return;
    }

    if (ThemeManager::instance().importTheme(filePath)) {
        QMessageBox::information(this, tr("Import Successful"), tr("Theme imported successfully."));
        // Reload the imported theme
        loadTheme();
    } else {
        QMessageBox::warning(this, tr("Import Failed"), tr("Failed to import theme from %1").arg(filePath));
    }
}

void ThemeEditorDialog::resetToDefaults()
{
    auto reply = QMessageBox::question(this, tr("Reset Theme"),
                                      tr("Reset to default dark theme?"),
                                      QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        setTheme(Theme::defaultDark());
    }
}

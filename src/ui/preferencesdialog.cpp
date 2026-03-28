#include "preferencesdialog.h"
#include "theme.h"
#include "utils/settings.h"
#include "core/encoding.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDialogButtonBox>

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
    loadSettings();
}

void PreferencesDialog::setupUi()
{
    setWindowTitle(tr("Preferences"));
    resize(500, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createEditorPage(), tr("Editor"));
    m_tabWidget->addTab(createAppearancePage(), tr("Appearance"));
    m_tabWidget->addTab(createFilesPage(), tr("Files"));

    mainLayout->addWidget(m_tabWidget);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_resetButton = new QPushButton(tr("Reset to Defaults"), this);
    m_applyButton = new QPushButton(tr("Apply"), this);
    QPushButton *okButton = new QPushButton(tr("OK"), this);
    QPushButton *cancelButton = new QPushButton(tr("Cancel"), this);

    okButton->setDefault(true);

    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_applyButton);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    connect(m_resetButton, &QPushButton::clicked, this, &PreferencesDialog::reset);
    connect(m_applyButton, &QPushButton::clicked, this, &PreferencesDialog::apply);
    connect(okButton, &QPushButton::clicked, this, &PreferencesDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

QWidget *PreferencesDialog::createEditorPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);

    // Font group
    QGroupBox *fontGroup = new QGroupBox(tr("Font"), page);
    QFormLayout *fontLayout = new QFormLayout(fontGroup);

    m_fontCombo = new QFontComboBox(fontGroup);
    m_fontCombo->setFontFilters(QFontComboBox::AllFonts);

    m_fontSizeSpin = new QSpinBox(fontGroup);
    m_fontSizeSpin->setRange(6, 72);
    m_fontSizeSpin->setSuffix(" pt");

    fontLayout->addRow(tr("Font family:"), m_fontCombo);
    fontLayout->addRow(tr("Font size:"), m_fontSizeSpin);

    layout->addWidget(fontGroup);

    // Indentation group
    QGroupBox *indentGroup = new QGroupBox(tr("Indentation"), page);
    QFormLayout *indentLayout = new QFormLayout(indentGroup);

    m_tabWidthSpin = new QSpinBox(indentGroup);
    m_tabWidthSpin->setRange(1, 16);

    m_insertSpacesCheck = new QCheckBox(tr("Insert spaces instead of tabs"), indentGroup);

    indentLayout->addRow(tr("Tab width:"), m_tabWidthSpin);
    indentLayout->addRow("", m_insertSpacesCheck);

    layout->addWidget(indentGroup);

    // Display group
    QGroupBox *displayGroup = new QGroupBox(tr("Display"), page);
    QVBoxLayout *displayLayout = new QVBoxLayout(displayGroup);

    m_wordWrapCheck = new QCheckBox(tr("Word wrap"), displayGroup);
    m_lineNumbersCheck = new QCheckBox(tr("Show line numbers"), displayGroup);
    m_highlightLineCheck = new QCheckBox(tr("Highlight current line"), displayGroup);
    m_minimapCheck = new QCheckBox(tr("Show minimap"), displayGroup);

    displayLayout->addWidget(m_wordWrapCheck);
    displayLayout->addWidget(m_lineNumbersCheck);
    displayLayout->addWidget(m_highlightLineCheck);
    displayLayout->addWidget(m_minimapCheck);

    layout->addWidget(displayGroup);
    layout->addStretch();

    return page;
}

QWidget *PreferencesDialog::createAppearancePage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);

    // Theme group
    QGroupBox *themeGroup = new QGroupBox(tr("Theme"), page);
    QFormLayout *themeLayout = new QFormLayout(themeGroup);

    m_themeCombo = new QComboBox(themeGroup);
    QVector<Theme> themes = ThemeManager::instance().themes();
    for (const Theme &t : themes) {
        m_themeCombo->addItem(t.name);
    }

    themeLayout->addRow(tr("Color theme:"), m_themeCombo);

    layout->addWidget(themeGroup);
    layout->addStretch();

    return page;
}

QWidget *PreferencesDialog::createFilesPage()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(page);

    // Encoding group
    QGroupBox *encodingGroup = new QGroupBox(tr("Encoding"), page);
    QFormLayout *encodingLayout = new QFormLayout(encodingGroup);

    m_encodingCombo = new QComboBox(encodingGroup);
    m_encodingCombo->addItems(Encoding::availableEncodings());

    m_lineEndingCombo = new QComboBox(encodingGroup);
    m_lineEndingCombo->addItem("Unix (LF)", "LF");
    m_lineEndingCombo->addItem("Windows (CRLF)", "CRLF");
    m_lineEndingCombo->addItem("Classic Mac (CR)", "CR");

    encodingLayout->addRow(tr("Default encoding:"), m_encodingCombo);
    encodingLayout->addRow(tr("Default line ending:"), m_lineEndingCombo);

    layout->addWidget(encodingGroup);

    // Save group
    QGroupBox *saveGroup = new QGroupBox(tr("Saving"), page);
    QVBoxLayout *saveLayout = new QVBoxLayout(saveGroup);

    m_trimWhitespaceCheck = new QCheckBox(tr("Trim trailing whitespace on save"), saveGroup);
    m_finalNewlineCheck = new QCheckBox(tr("Insert final newline on save"), saveGroup);

    QHBoxLayout *autoSaveLayout = new QHBoxLayout();
    m_autoSaveCheck = new QCheckBox(tr("Auto-save every"), saveGroup);
    m_autoSaveIntervalSpin = new QSpinBox(saveGroup);
    m_autoSaveIntervalSpin->setRange(1, 60);
    m_autoSaveIntervalSpin->setSuffix(" seconds");
    autoSaveLayout->addWidget(m_autoSaveCheck);
    autoSaveLayout->addWidget(m_autoSaveIntervalSpin);
    autoSaveLayout->addStretch();

    connect(m_autoSaveCheck, &QCheckBox::toggled, m_autoSaveIntervalSpin, &QWidget::setEnabled);

    saveLayout->addWidget(m_trimWhitespaceCheck);
    saveLayout->addWidget(m_finalNewlineCheck);
    saveLayout->addLayout(autoSaveLayout);

    layout->addWidget(saveGroup);

    // Session group
    QGroupBox *sessionGroup = new QGroupBox(tr("Session"), page);
    QVBoxLayout *sessionLayout = new QVBoxLayout(sessionGroup);

    m_restoreSessionCheck = new QCheckBox(tr("Restore previous session on startup"), sessionGroup);

    sessionLayout->addWidget(m_restoreSessionCheck);

    layout->addWidget(sessionGroup);
    layout->addStretch();

    return page;
}

void PreferencesDialog::loadSettings()
{
    Settings &s = Settings::instance();

    // Editor
    m_fontCombo->setCurrentFont(QFont(s.fontFamily()));
    m_fontSizeSpin->setValue(s.fontSize());
    m_tabWidthSpin->setValue(s.tabWidth());
    m_insertSpacesCheck->setChecked(s.insertSpaces());
    m_wordWrapCheck->setChecked(s.wordWrap());
    m_lineNumbersCheck->setChecked(s.showLineNumbers());
    m_highlightLineCheck->setChecked(s.highlightCurrentLine());
    m_minimapCheck->setChecked(s.showMinimap());

    // Appearance
    int themeIndex = m_themeCombo->findText(s.theme());
    if (themeIndex >= 0) {
        m_themeCombo->setCurrentIndex(themeIndex);
    }

    // Files
    int encodingIndex = m_encodingCombo->findText(Encoding::displayName(s.defaultEncoding()));
    if (encodingIndex >= 0) {
        m_encodingCombo->setCurrentIndex(encodingIndex);
    }

    int lineEndingIndex = m_lineEndingCombo->findData(s.defaultLineEnding());
    if (lineEndingIndex >= 0) {
        m_lineEndingCombo->setCurrentIndex(lineEndingIndex);
    }

    m_trimWhitespaceCheck->setChecked(s.trimTrailingWhitespace());
    m_finalNewlineCheck->setChecked(s.insertFinalNewline());
    m_autoSaveCheck->setChecked(s.autoSave());
    m_autoSaveIntervalSpin->setValue(s.autoSaveInterval() / 1000);
    m_autoSaveIntervalSpin->setEnabled(s.autoSave());
    m_restoreSessionCheck->setChecked(s.restoreSession());
}

void PreferencesDialog::saveSettings()
{
    Settings &s = Settings::instance();

    // Editor
    s.setFontFamily(m_fontCombo->currentFont().family());
    s.setFontSize(m_fontSizeSpin->value());
    s.setTabWidth(m_tabWidthSpin->value());
    s.setInsertSpaces(m_insertSpacesCheck->isChecked());
    s.setWordWrap(m_wordWrapCheck->isChecked());
    s.setShowLineNumbers(m_lineNumbersCheck->isChecked());
    s.setHighlightCurrentLine(m_highlightLineCheck->isChecked());
    s.setShowMinimap(m_minimapCheck->isChecked());

    // Appearance
    s.setTheme(m_themeCombo->currentText());

    // Files
    s.setDefaultEncoding(Encoding::codecName(m_encodingCombo->currentText()));
    s.setDefaultLineEnding(m_lineEndingCombo->currentData().toString());
    s.setTrimTrailingWhitespace(m_trimWhitespaceCheck->isChecked());
    s.setInsertFinalNewline(m_finalNewlineCheck->isChecked());
    s.setAutoSave(m_autoSaveCheck->isChecked());
    s.setAutoSaveInterval(m_autoSaveIntervalSpin->value() * 1000);
    s.setRestoreSession(m_restoreSessionCheck->isChecked());
}

void PreferencesDialog::apply()
{
    saveSettings();
    Settings::instance().save();
}

void PreferencesDialog::reset()
{
    Settings::instance().reset();
    loadSettings();
}

void PreferencesDialog::accept()
{
    saveSettings();
    Settings::instance().save();
    QDialog::accept();
}

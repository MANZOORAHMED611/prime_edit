#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QFontComboBox>
#include <QPushButton>

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);

private slots:
    void apply();
    void reset();
    void accept() override;

private:
    void setupUi();
    void loadSettings();
    void saveSettings();

    QWidget *createEditorPage();
    QWidget *createAppearancePage();
    QWidget *createFilesPage();

    QTabWidget *m_tabWidget;

    // Editor settings
    QFontComboBox *m_fontCombo;
    QSpinBox *m_fontSizeSpin;
    QSpinBox *m_tabWidthSpin;
    QCheckBox *m_insertSpacesCheck;
    QCheckBox *m_wordWrapCheck;
    QCheckBox *m_lineNumbersCheck;
    QCheckBox *m_highlightLineCheck;
    QCheckBox *m_minimapCheck;

    // Appearance settings
    QComboBox *m_themeCombo;

    // File settings
    QComboBox *m_encodingCombo;
    QComboBox *m_lineEndingCombo;
    QCheckBox *m_trimWhitespaceCheck;
    QCheckBox *m_finalNewlineCheck;
    QCheckBox *m_autoSaveCheck;
    QSpinBox *m_autoSaveIntervalSpin;
    QCheckBox *m_restoreSessionCheck;

    QPushButton *m_applyButton;
    QPushButton *m_resetButton;
};

#endif // PREFERENCESDIALOG_H

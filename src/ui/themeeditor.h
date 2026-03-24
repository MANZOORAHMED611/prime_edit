#ifndef THEMEEDITOR_H
#define THEMEEDITOR_H

#include <QDialog>
#include <QColor>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QTextEdit>
#include <QMap>
#include "theme.h"

class ColorButton : public QPushButton
{
    Q_OBJECT
public:
    explicit ColorButton(QWidget *parent = nullptr);

    QColor color() const { return m_color; }
    void setColor(const QColor &color);

signals:
    void colorChanged(const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QColor m_color;
};

class ThemeEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ThemeEditorDialog(QWidget *parent = nullptr);
    ~ThemeEditorDialog() override = default;

    void setTheme(const Theme &theme);
    Theme theme() const { return m_theme; }

private slots:
    void onColorChanged();
    void onMetadataChanged();
    void updatePreview();
    void saveTheme();
    void loadTheme();
    void exportTheme();
    void importTheme();
    void resetToDefaults();

private:
    void setupUI();
    void createMetadataSection(QWidget *container);
    void createEditorColorsSection(QWidget *container);
    void createSyntaxColorsSection(QWidget *container);
    void createUIColorsSection(QWidget *container);
    void createAccentColorsSection(QWidget *container);
    void createDiagnosticColorsSection(QWidget *container);
    void createPreviewSection(QWidget *container);
    void createButtonSection(QWidget *container);

    ColorButton* createColorButton(const QString &label, const QColor &color, QWidget *parent);
    void connectColorButton(ColorButton *button, QColor Theme::*member);

    Theme m_theme;

    // Metadata widgets
    QLineEdit *m_nameEdit;
    QLineEdit *m_authorEdit;
    QCheckBox *m_isDarkCheckbox;

    // Color buttons - Editor
    ColorButton *m_backgroundBtn;
    ColorButton *m_foregroundBtn;
    ColorButton *m_selectionBgBtn;
    ColorButton *m_selectionFgBtn;
    ColorButton *m_currentLineBgBtn;
    ColorButton *m_lineNumberFgBtn;
    ColorButton *m_lineNumberBgBtn;

    // Color buttons - Syntax
    ColorButton *m_keywordBtn;
    ColorButton *m_stringBtn;
    ColorButton *m_commentBtn;
    ColorButton *m_numberBtn;
    ColorButton *m_functionBtn;
    ColorButton *m_typeBtn;
    ColorButton *m_variableBtn;
    ColorButton *m_operatorBtn;

    // Color buttons - UI
    ColorButton *m_menuBgBtn;
    ColorButton *m_menuFgBtn;
    ColorButton *m_toolbarBgBtn;
    ColorButton *m_statusBarBgBtn;
    ColorButton *m_statusBarFgBtn;
    ColorButton *m_tabActiveBgBtn;
    ColorButton *m_tabInactiveBgBtn;
    ColorButton *m_tabFgBtn;

    // Color buttons - Accents
    ColorButton *m_accentPrimaryBtn;
    ColorButton *m_accentSecondaryBtn;
    ColorButton *m_borderColorBtn;

    // Color buttons - Diagnostics
    ColorButton *m_errorFgBtn;
    ColorButton *m_warningFgBtn;
    ColorButton *m_infoFgBtn;
    ColorButton *m_hintFgBtn;

    // Preview
    QTextEdit *m_previewEdit;
    QWidget *m_previewContainer;
};

#endif // THEMEEDITOR_H

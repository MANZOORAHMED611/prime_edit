#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QRadioButton>
#include <QPushButton>
#include <QButtonGroup>
#include <QLabel>
#include <QSlider>
#include "core/searchengine.h"

class SearchDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SearchDialog(QWidget *parent = nullptr);

    void setSearchText(const QString &text);
    void showTab(int index);

    QString searchText() const;
    QString replaceText() const;
    SearchEngine::Options searchOptions() const;

    // Find in Files accessors
    QString directory() const;
    QString fileFilter() const;
    bool recursive() const;
    bool includeHidden() const;

signals:
    void findNext();
    void findPrevious();
    void replaceOne();
    void replaceAll();
    void countRequested();
    void findAllInCurrent();
    void findAllInAllOpen();
    void findInFiles();
    void replaceInFiles();
    void markAll();
    void clearMarks();

private:
    void setupFindTab();
    void setupReplaceTab();
    void setupFindInFilesTab();
    void setupMarkTab();
    void addToHistory(const QString &text);

    QTabWidget *m_tabWidget;

    // Find tab
    QComboBox *m_findCombo;
    QCheckBox *m_matchCase;
    QCheckBox *m_wholeWord;
    QCheckBox *m_wrapAround;
    QCheckBox *m_dotMatchesNewline;
    QRadioButton *m_normalMode;
    QRadioButton *m_extendedMode;
    QRadioButton *m_regexMode;
    QRadioButton *m_directionUp;
    QRadioButton *m_directionDown;

    // Replace tab
    QComboBox *m_replaceCombo;
    QComboBox *m_replaceFindCombo;

    // Find in Files tab
    QComboBox *m_filesFindCombo;
    QLineEdit *m_directoryEdit;
    QLineEdit *m_filterEdit;
    QCheckBox *m_recursiveCheck;
    QCheckBox *m_hiddenCheck;

    // Mark tab
    QComboBox *m_markFindCombo;
    QCheckBox *m_bookmarkLine;

    // Shared
    QLabel *m_statusLabel;
    QSlider *m_transparencySlider;

    QStringList m_searchHistory;
};

#endif

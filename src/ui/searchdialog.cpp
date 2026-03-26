#include "searchdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QKeyEvent>

SearchDialog::SearchDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Search"));
    setMinimumWidth(520);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_tabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_tabWidget);

    setupFindTab();
    setupReplaceTab();
    setupFindInFilesTab();
    setupMarkTab();

    // Status label
    m_statusLabel = new QLabel(this);
    mainLayout->addWidget(m_statusLabel);

    // Transparency slider at bottom
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    QLabel *transLabel = new QLabel(tr("Transparency:"), this);
    bottomLayout->addWidget(transLabel);
    m_transparencySlider = new QSlider(Qt::Horizontal, this);
    m_transparencySlider->setRange(20, 100);
    m_transparencySlider->setValue(100);
    m_transparencySlider->setFixedWidth(120);
    bottomLayout->addWidget(m_transparencySlider);
    mainLayout->addLayout(bottomLayout);

    connect(m_transparencySlider, &QSlider::valueChanged, this, [this](int value) {
        setWindowOpacity(value / 100.0);
    });
}

void SearchDialog::setupFindTab()
{
    QWidget *findTab = new QWidget();
    QGridLayout *grid = new QGridLayout(findTab);

    // Row 0: Find what
    grid->addWidget(new QLabel(tr("Find what:"), findTab), 0, 0);
    m_findCombo = new QComboBox(findTab);
    m_findCombo->setEditable(true);
    m_findCombo->setMinimumWidth(300);
    m_findCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    grid->addWidget(m_findCombo, 0, 1, 1, 2);

    // Row 1: Options group and search mode group
    QGroupBox *optionsGroup = new QGroupBox(tr("Options"), findTab);
    QVBoxLayout *optLay = new QVBoxLayout(optionsGroup);
    m_matchCase = new QCheckBox(tr("Match &case"), optionsGroup);
    m_wholeWord = new QCheckBox(tr("Match &whole word only"), optionsGroup);
    m_wrapAround = new QCheckBox(tr("Wrap ar&ound"), optionsGroup);
    m_wrapAround->setChecked(true);
    m_dotMatchesNewline = new QCheckBox(tr("&. matches newline"), optionsGroup);
    optLay->addWidget(m_matchCase);
    optLay->addWidget(m_wholeWord);
    optLay->addWidget(m_wrapAround);
    optLay->addWidget(m_dotMatchesNewline);
    grid->addWidget(optionsGroup, 1, 0, 2, 1);

    QGroupBox *modeGroup = new QGroupBox(tr("Search Mode"), findTab);
    QVBoxLayout *modeLay = new QVBoxLayout(modeGroup);
    m_normalMode = new QRadioButton(tr("&Normal"), modeGroup);
    m_extendedMode = new QRadioButton(tr("&Extended (\\n, \\r, \\t, \\0, \\x...)"), modeGroup);
    m_regexMode = new QRadioButton(tr("Regular &expression"), modeGroup);
    m_normalMode->setChecked(true);
    QButtonGroup *modeBtnGroup = new QButtonGroup(this);
    modeBtnGroup->addButton(m_normalMode);
    modeBtnGroup->addButton(m_extendedMode);
    modeBtnGroup->addButton(m_regexMode);
    modeLay->addWidget(m_normalMode);
    modeLay->addWidget(m_extendedMode);
    modeLay->addWidget(m_regexMode);
    grid->addWidget(modeGroup, 1, 1);

    QGroupBox *dirGroup = new QGroupBox(tr("Direction"), findTab);
    QHBoxLayout *dirLay = new QHBoxLayout(dirGroup);
    m_directionUp = new QRadioButton(tr("&Up"), dirGroup);
    m_directionDown = new QRadioButton(tr("&Down"), dirGroup);
    m_directionDown->setChecked(true);
    QButtonGroup *dirBtnGroup = new QButtonGroup(this);
    dirBtnGroup->addButton(m_directionUp);
    dirBtnGroup->addButton(m_directionDown);
    dirLay->addWidget(m_directionUp);
    dirLay->addWidget(m_directionDown);
    grid->addWidget(dirGroup, 2, 1);

    // Row 3: Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *findNextBtn = new QPushButton(tr("Find &Next"), findTab);
    findNextBtn->setDefault(true);
    QPushButton *findPrevBtn = new QPushButton(tr("Find &Previous"), findTab);
    QPushButton *countBtn = new QPushButton(tr("Coun&t"), findTab);
    QPushButton *findAllCurrentBtn = new QPushButton(tr("Find All in Current"), findTab);
    QPushButton *findAllOpenBtn = new QPushButton(tr("Find All in All Open"), findTab);
    QPushButton *closeBtn = new QPushButton(tr("Close"), findTab);

    btnLayout->addWidget(findNextBtn);
    btnLayout->addWidget(findPrevBtn);
    btnLayout->addWidget(countBtn);
    btnLayout->addWidget(findAllCurrentBtn);
    btnLayout->addWidget(findAllOpenBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    grid->addLayout(btnLayout, 3, 0, 1, 3);

    connect(findNextBtn, &QPushButton::clicked, this, [this]() {
        addToHistory(m_findCombo->currentText());
        emit findNext();
    });
    connect(findPrevBtn, &QPushButton::clicked, this, [this]() {
        addToHistory(m_findCombo->currentText());
        emit findPrevious();
    });
    connect(countBtn, &QPushButton::clicked, this, [this]() {
        addToHistory(m_findCombo->currentText());
        emit countRequested();
    });
    connect(findAllCurrentBtn, &QPushButton::clicked, this, [this]() {
        addToHistory(m_findCombo->currentText());
        emit findAllInCurrent();
    });
    connect(findAllOpenBtn, &QPushButton::clicked, this, [this]() {
        addToHistory(m_findCombo->currentText());
        emit findAllInAllOpen();
    });
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);

    m_tabWidget->addTab(findTab, tr("Find"));
}

void SearchDialog::setupReplaceTab()
{
    QWidget *replaceTab = new QWidget();
    QGridLayout *grid = new QGridLayout(replaceTab);

    // Row 0: Find what
    grid->addWidget(new QLabel(tr("Find what:"), replaceTab), 0, 0);
    m_replaceFindCombo = new QComboBox(replaceTab);
    m_replaceFindCombo->setEditable(true);
    m_replaceFindCombo->setMinimumWidth(300);
    m_replaceFindCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    grid->addWidget(m_replaceFindCombo, 0, 1, 1, 2);

    // Row 1: Replace with
    grid->addWidget(new QLabel(tr("Replace with:"), replaceTab), 1, 0);
    m_replaceCombo = new QComboBox(replaceTab);
    m_replaceCombo->setEditable(true);
    m_replaceCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    grid->addWidget(m_replaceCombo, 1, 1, 1, 2);

    // Row 2: Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *findNextBtn = new QPushButton(tr("Find &Next"), replaceTab);
    QPushButton *replaceBtn = new QPushButton(tr("&Replace"), replaceTab);
    QPushButton *replaceAllBtn = new QPushButton(tr("Replace &All"), replaceTab);
    QPushButton *closeBtn = new QPushButton(tr("Close"), replaceTab);

    btnLayout->addWidget(findNextBtn);
    btnLayout->addWidget(replaceBtn);
    btnLayout->addWidget(replaceAllBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    grid->addLayout(btnLayout, 2, 0, 1, 3);

    connect(findNextBtn, &QPushButton::clicked, this, [this]() {
        addToHistory(m_replaceFindCombo->currentText());
        emit findNext();
    });
    connect(replaceBtn, &QPushButton::clicked, this, [this]() {
        addToHistory(m_replaceFindCombo->currentText());
        emit replaceOne();
    });
    connect(replaceAllBtn, &QPushButton::clicked, this, [this]() {
        addToHistory(m_replaceFindCombo->currentText());
        emit replaceAll();
    });
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);

    m_tabWidget->addTab(replaceTab, tr("Replace"));
}

void SearchDialog::setupFindInFilesTab()
{
    QWidget *filesTab = new QWidget();
    QGridLayout *grid = new QGridLayout(filesTab);

    // Row 0: Find what
    grid->addWidget(new QLabel(tr("Find what:"), filesTab), 0, 0);
    m_filesFindCombo = new QComboBox(filesTab);
    m_filesFindCombo->setEditable(true);
    m_filesFindCombo->setMinimumWidth(300);
    m_filesFindCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    grid->addWidget(m_filesFindCombo, 0, 1, 1, 2);

    // Row 1: Directory
    grid->addWidget(new QLabel(tr("Directory:"), filesTab), 1, 0);
    m_directoryEdit = new QLineEdit(filesTab);
    grid->addWidget(m_directoryEdit, 1, 1);
    QPushButton *browseBtn = new QPushButton(tr("..."), filesTab);
    browseBtn->setFixedWidth(30);
    grid->addWidget(browseBtn, 1, 2);

    // Row 2: Filters
    grid->addWidget(new QLabel(tr("Filters:"), filesTab), 2, 0);
    m_filterEdit = new QLineEdit(filesTab);
    m_filterEdit->setText("*.*");
    grid->addWidget(m_filterEdit, 2, 1, 1, 2);

    // Row 3: Options
    QHBoxLayout *optLayout = new QHBoxLayout();
    m_recursiveCheck = new QCheckBox(tr("In all sub-folders"), filesTab);
    m_recursiveCheck->setChecked(true);
    m_hiddenCheck = new QCheckBox(tr("In hidden folders"), filesTab);
    optLayout->addWidget(m_recursiveCheck);
    optLayout->addWidget(m_hiddenCheck);
    optLayout->addStretch();
    grid->addLayout(optLayout, 3, 0, 1, 3);

    // Row 4: Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *findAllBtn = new QPushButton(tr("Find All"), filesTab);
    QPushButton *replaceInFilesBtn = new QPushButton(tr("Replace in Files"), filesTab);
    QPushButton *closeBtn = new QPushButton(tr("Close"), filesTab);
    btnLayout->addWidget(findAllBtn);
    btnLayout->addWidget(replaceInFilesBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    grid->addLayout(btnLayout, 4, 0, 1, 3);

    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Directory"),
                                                         m_directoryEdit->text());
        if (!dir.isEmpty()) {
            m_directoryEdit->setText(dir);
        }
    });
    connect(findAllBtn, &QPushButton::clicked, this, [this]() {
        addToHistory(m_filesFindCombo->currentText());
        emit findInFiles();
    });
    connect(replaceInFilesBtn, &QPushButton::clicked, this, [this]() {
        addToHistory(m_filesFindCombo->currentText());
        emit replaceInFiles();
    });
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);

    m_tabWidget->addTab(filesTab, tr("Find in Files"));
}

void SearchDialog::setupMarkTab()
{
    QWidget *markTab = new QWidget();
    QGridLayout *grid = new QGridLayout(markTab);

    // Row 0: Find what
    grid->addWidget(new QLabel(tr("Find what:"), markTab), 0, 0);
    m_markFindCombo = new QComboBox(markTab);
    m_markFindCombo->setEditable(true);
    m_markFindCombo->setMinimumWidth(300);
    m_markFindCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    grid->addWidget(m_markFindCombo, 0, 1, 1, 2);

    // Row 1: Options
    m_bookmarkLine = new QCheckBox(tr("Bookmark line"), markTab);
    grid->addWidget(m_bookmarkLine, 1, 0, 1, 2);

    // Row 2: Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *markAllBtn = new QPushButton(tr("Mark All"), markTab);
    QPushButton *clearMarksBtn = new QPushButton(tr("Clear All Marks"), markTab);
    QPushButton *closeBtn = new QPushButton(tr("Close"), markTab);
    btnLayout->addWidget(markAllBtn);
    btnLayout->addWidget(clearMarksBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    grid->addLayout(btnLayout, 2, 0, 1, 3);

    connect(markAllBtn, &QPushButton::clicked, this, [this]() {
        addToHistory(m_markFindCombo->currentText());
        emit markAll();
    });
    connect(clearMarksBtn, &QPushButton::clicked, this, &SearchDialog::clearMarks);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);

    m_tabWidget->addTab(markTab, tr("Mark"));
}

void SearchDialog::setSearchText(const QString &text)
{
    m_findCombo->setCurrentText(text);
    m_replaceFindCombo->setCurrentText(text);
    m_filesFindCombo->setCurrentText(text);
    m_markFindCombo->setCurrentText(text);
}

void SearchDialog::showTab(int index)
{
    if (index >= 0 && index < m_tabWidget->count()) {
        m_tabWidget->setCurrentIndex(index);
    }
}

QString SearchDialog::searchText() const
{
    int tab = m_tabWidget->currentIndex();
    switch (tab) {
    case 1:  return m_replaceFindCombo->currentText();
    case 2:  return m_filesFindCombo->currentText();
    case 3:  return m_markFindCombo->currentText();
    default: return m_findCombo->currentText();
    }
}

QString SearchDialog::replaceText() const
{
    return m_replaceCombo->currentText();
}

SearchEngine::Options SearchDialog::searchOptions() const
{
    SearchEngine::Options opts;
    opts.caseSensitive = m_matchCase->isChecked();
    opts.wholeWord = m_wholeWord->isChecked();
    opts.wrapAround = m_wrapAround->isChecked();
    opts.dotMatchesNewline = m_dotMatchesNewline->isChecked();
    opts.searchBackward = m_directionUp->isChecked();

    if (m_regexMode->isChecked()) {
        opts.mode = SearchEngine::Regex;
    } else if (m_extendedMode->isChecked()) {
        opts.mode = SearchEngine::Extended;
    } else {
        opts.mode = SearchEngine::Normal;
    }

    return opts;
}

QString SearchDialog::directory() const
{
    return m_directoryEdit->text();
}

QString SearchDialog::fileFilter() const
{
    return m_filterEdit->text();
}

bool SearchDialog::recursive() const
{
    return m_recursiveCheck->isChecked();
}

bool SearchDialog::includeHidden() const
{
    return m_hiddenCheck->isChecked();
}

void SearchDialog::addToHistory(const QString &text)
{
    if (text.isEmpty()) return;

    // Add to all combo boxes if not already present
    QList<QComboBox*> combos = {m_findCombo, m_replaceFindCombo, m_filesFindCombo, m_markFindCombo};
    for (QComboBox *combo : combos) {
        int idx = combo->findText(text);
        if (idx >= 0) {
            combo->removeItem(idx);
        }
        combo->insertItem(0, text);
        if (combo->count() > 20) {
            combo->removeItem(combo->count() - 1);
        }
        combo->setCurrentText(text);
    }

    if (!m_searchHistory.contains(text)) {
        m_searchHistory.prepend(text);
        if (m_searchHistory.size() > 20) {
            m_searchHistory.removeLast();
        }
    }
}

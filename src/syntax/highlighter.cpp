#include "highlighter.h"

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    setupFormats();
}

void SyntaxHighlighter::setupFormats()
{
    // Keywords (blue)
    m_keywordFormat.setForeground(QColor(86, 156, 214));
    m_keywordFormat.setFontWeight(QFont::Bold);

    // Types (cyan)
    m_typeFormat.setForeground(QColor(78, 201, 176));

    // Functions (yellow)
    m_functionFormat.setForeground(QColor(220, 220, 170));

    // Strings (orange)
    m_stringFormat.setForeground(QColor(206, 145, 120));

    // Numbers (light green)
    m_numberFormat.setForeground(QColor(181, 206, 168));

    // Comments (green)
    m_commentFormat.setForeground(QColor(106, 153, 85));
    m_commentFormat.setFontItalic(true);

    // Preprocessor (magenta)
    m_preprocessorFormat.setForeground(QColor(197, 134, 192));

    // Operators
    m_operatorFormat.setForeground(QColor(212, 212, 212));

    // HTML/XML tags
    m_tagFormat.setForeground(QColor(86, 156, 214));

    // Attributes
    m_attributeFormat.setForeground(QColor(156, 220, 254));

    // Variables
    m_variableFormat.setForeground(QColor(156, 220, 254));
}

void SyntaxHighlighter::setLanguage(const QString &language)
{
    if (m_language == language) {
        return;
    }

    m_language = language;
    m_rules.clear();
    loadLanguageRules(language);
    rehighlight();
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    // Apply single-line rules
    for (const HighlightingRule &rule : m_rules) {
        QRegularExpressionMatchIterator i = rule.pattern.globalMatch(text);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // Handle multi-line comments
    setCurrentBlockState(0);

    if (!m_commentStartExpression.pattern().isEmpty()) {
        int startIndex = 0;
        if (previousBlockState() != 1) {
            startIndex = text.indexOf(m_commentStartExpression);
        }

        while (startIndex >= 0) {
            QRegularExpressionMatch endMatch = m_commentEndExpression.match(text, startIndex + 2);
            int endIndex = endMatch.capturedStart();
            int commentLength;

            if (endIndex == -1) {
                setCurrentBlockState(1);
                commentLength = text.length() - startIndex;
            } else {
                commentLength = endIndex - startIndex + endMatch.capturedLength();
            }

            setFormat(startIndex, commentLength, m_commentFormat);
            startIndex = text.indexOf(m_commentStartExpression, startIndex + commentLength);
        }
    }
}

void SyntaxHighlighter::loadLanguageRules(const QString &language)
{
    QString lang = language.toLower();

    if (lang == "c++" || lang == "cpp" || lang == "c" || lang == "h" || lang == "hpp") {
        loadCppRules();
    } else if (lang == "python" || lang == "py") {
        loadPythonRules();
    } else if (lang == "javascript" || lang == "js" || lang == "typescript" || lang == "ts") {
        loadJavaScriptRules();
    } else if (lang == "html" || lang == "htm" || lang == "xml") {
        loadHtmlRules();
    } else if (lang == "css" || lang == "scss" || lang == "sass") {
        loadCssRules();
    } else if (lang == "json") {
        loadJsonRules();
    } else if (lang == "yaml" || lang == "yml") {
        loadYamlRules();
    } else if (lang == "markdown" || lang == "md") {
        loadMarkdownRules();
    } else if (lang == "bash" || lang == "sh" || lang == "shell") {
        loadBashRules();
    } else if (lang == "rust" || lang == "rs") {
        loadRustRules();
    } else if (lang == "go") {
        loadGoRules();
    } else if (lang == "java") {
        loadJavaRules();
    } else if (lang == "sql") {
        loadSqlRules();
    }
}

void SyntaxHighlighter::loadCppRules()
{
    HighlightingRule rule;

    // Keywords
    QStringList keywordPatterns = {
        "\\balignas\\b", "\\balignof\\b", "\\band\\b", "\\band_eq\\b", "\\basm\\b",
        "\\bauto\\b", "\\bbitand\\b", "\\bbitor\\b", "\\bbool\\b", "\\bbreak\\b",
        "\\bcase\\b", "\\bcatch\\b", "\\bchar\\b", "\\bchar16_t\\b", "\\bchar32_t\\b",
        "\\bclass\\b", "\\bcompl\\b", "\\bconst\\b", "\\bconstexpr\\b", "\\bconst_cast\\b",
        "\\bcontinue\\b", "\\bdecltype\\b", "\\bdefault\\b", "\\bdelete\\b", "\\bdo\\b",
        "\\bdouble\\b", "\\bdynamic_cast\\b", "\\belse\\b", "\\benum\\b", "\\bexplicit\\b",
        "\\bexport\\b", "\\bextern\\b", "\\bfalse\\b", "\\bfloat\\b", "\\bfor\\b",
        "\\bfriend\\b", "\\bgoto\\b", "\\bif\\b", "\\binline\\b", "\\bint\\b",
        "\\blong\\b", "\\bmutable\\b", "\\bnamespace\\b", "\\bnew\\b", "\\bnoexcept\\b",
        "\\bnot\\b", "\\bnot_eq\\b", "\\bnullptr\\b", "\\boperator\\b", "\\bor\\b",
        "\\bor_eq\\b", "\\bprivate\\b", "\\bprotected\\b", "\\bpublic\\b", "\\bregister\\b",
        "\\breinterpret_cast\\b", "\\breturn\\b", "\\bshort\\b", "\\bsigned\\b", "\\bsizeof\\b",
        "\\bstatic\\b", "\\bstatic_assert\\b", "\\bstatic_cast\\b", "\\bstruct\\b", "\\bswitch\\b",
        "\\btemplate\\b", "\\bthis\\b", "\\bthread_local\\b", "\\bthrow\\b", "\\btrue\\b",
        "\\btry\\b", "\\btypedef\\b", "\\btypeid\\b", "\\btypename\\b", "\\bunion\\b",
        "\\bunsigned\\b", "\\busing\\b", "\\bvirtual\\b", "\\bvoid\\b", "\\bvolatile\\b",
        "\\bwchar_t\\b", "\\bwhile\\b", "\\bxor\\b", "\\bxor_eq\\b", "\\boverride\\b", "\\bfinal\\b"
    };

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    // Types
    QStringList typePatterns = {
        "\\bsize_t\\b", "\\bint8_t\\b", "\\bint16_t\\b", "\\bint32_t\\b", "\\bint64_t\\b",
        "\\buint8_t\\b", "\\buint16_t\\b", "\\buint32_t\\b", "\\buint64_t\\b",
        "\\bstring\\b", "\\bvector\\b", "\\bmap\\b", "\\bset\\b", "\\blist\\b",
        "\\barray\\b", "\\bpair\\b", "\\btuple\\b", "\\bunique_ptr\\b", "\\bshared_ptr\\b"
    };

    for (const QString &pattern : typePatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_typeFormat;
        m_rules.append(rule);
    }

    // Functions
    rule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\s*\\()");
    rule.format = m_functionFormat;
    m_rules.append(rule);

    // Preprocessor
    rule.pattern = QRegularExpression("^\\s*#[^\n]*");
    rule.format = m_preprocessorFormat;
    m_rules.append(rule);

    // Numbers
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*[fFlLuU]*\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);

    // Hex numbers
    rule.pattern = QRegularExpression("\\b0x[0-9A-Fa-f]+\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);

    // Strings
    rule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    // Character literals
    rule.pattern = QRegularExpression("'[^'\\\\]*(\\\\.[^'\\\\]*)*'");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    // Single-line comments
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = m_commentFormat;
    m_rules.append(rule);

    // Multi-line comments
    m_commentStartExpression = QRegularExpression("/\\*");
    m_commentEndExpression = QRegularExpression("\\*/");
}

void SyntaxHighlighter::loadPythonRules()
{
    HighlightingRule rule;

    // Keywords
    QStringList keywordPatterns = {
        "\\band\\b", "\\bas\\b", "\\bassert\\b", "\\basync\\b", "\\bawait\\b",
        "\\bbreak\\b", "\\bclass\\b", "\\bcontinue\\b", "\\bdef\\b", "\\bdel\\b",
        "\\belif\\b", "\\belse\\b", "\\bexcept\\b", "\\bfinally\\b", "\\bfor\\b",
        "\\bfrom\\b", "\\bglobal\\b", "\\bif\\b", "\\bimport\\b", "\\bin\\b",
        "\\bis\\b", "\\blambda\\b", "\\bnonlocal\\b", "\\bnot\\b", "\\bor\\b",
        "\\bpass\\b", "\\braise\\b", "\\breturn\\b", "\\btry\\b", "\\bwhile\\b",
        "\\bwith\\b", "\\byield\\b", "\\bTrue\\b", "\\bFalse\\b", "\\bNone\\b"
    };

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    // Built-in types
    QStringList typePatterns = {
        "\\bint\\b", "\\bfloat\\b", "\\bstr\\b", "\\bbool\\b", "\\blist\\b",
        "\\bdict\\b", "\\btuple\\b", "\\bset\\b", "\\bfrozenset\\b", "\\bbytes\\b",
        "\\bbytearray\\b", "\\bobject\\b", "\\btype\\b", "\\bException\\b"
    };

    for (const QString &pattern : typePatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_typeFormat;
        m_rules.append(rule);
    }

    // Functions
    rule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\s*\\()");
    rule.format = m_functionFormat;
    m_rules.append(rule);

    // Decorators
    rule.pattern = QRegularExpression("@[A-Za-z_][A-Za-z0-9_]*");
    rule.format = m_preprocessorFormat;
    m_rules.append(rule);

    // Self
    rule.pattern = QRegularExpression("\\bself\\b");
    rule.format = m_variableFormat;
    m_rules.append(rule);

    // Numbers
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*[jJ]?\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);

    // Strings
    rule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"|'[^'\\\\]*(\\\\.[^'\\\\]*)*'");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    // Triple-quoted strings
    rule.pattern = QRegularExpression("\"\"\"[^\"]*\"\"\"|'''[^']*'''");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    // Comments
    rule.pattern = QRegularExpression("#[^\n]*");
    rule.format = m_commentFormat;
    m_rules.append(rule);
}

void SyntaxHighlighter::loadJavaScriptRules()
{
    HighlightingRule rule;

    // Keywords
    QStringList keywordPatterns = {
        "\\bbreak\\b", "\\bcase\\b", "\\bcatch\\b", "\\bcontinue\\b", "\\bdebugger\\b",
        "\\bdefault\\b", "\\bdelete\\b", "\\bdo\\b", "\\belse\\b", "\\bfinally\\b",
        "\\bfor\\b", "\\bfunction\\b", "\\bif\\b", "\\bin\\b", "\\binstanceof\\b",
        "\\bnew\\b", "\\breturn\\b", "\\bswitch\\b", "\\bthis\\b", "\\bthrow\\b",
        "\\btry\\b", "\\btypeof\\b", "\\bvar\\b", "\\bvoid\\b", "\\bwhile\\b",
        "\\bwith\\b", "\\bclass\\b", "\\bconst\\b", "\\benum\\b", "\\bexport\\b",
        "\\bextends\\b", "\\bimport\\b", "\\bsuper\\b", "\\blet\\b", "\\bstatic\\b",
        "\\byield\\b", "\\bawait\\b", "\\basync\\b", "\\btrue\\b", "\\bfalse\\b",
        "\\bnull\\b", "\\bundefined\\b", "\\bNaN\\b", "\\bInfinity\\b"
    };

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    // Functions
    rule.pattern = QRegularExpression("\\b[A-Za-z_$][A-Za-z0-9_$]*(?=\\s*\\()");
    rule.format = m_functionFormat;
    m_rules.append(rule);

    // Numbers
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*([eE][+-]?[0-9]+)?\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);

    // Strings
    rule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"|'[^'\\\\]*(\\\\.[^'\\\\]*)*'|`[^`\\\\]*(\\\\.[^`\\\\]*)*`");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    // Comments
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = m_commentFormat;
    m_rules.append(rule);

    m_commentStartExpression = QRegularExpression("/\\*");
    m_commentEndExpression = QRegularExpression("\\*/");
}

void SyntaxHighlighter::loadHtmlRules()
{
    HighlightingRule rule;

    // Tags
    rule.pattern = QRegularExpression("</?[A-Za-z][A-Za-z0-9]*");
    rule.format = m_tagFormat;
    m_rules.append(rule);

    // Closing bracket
    rule.pattern = QRegularExpression("/?>");
    rule.format = m_tagFormat;
    m_rules.append(rule);

    // Attributes
    rule.pattern = QRegularExpression("\\b[A-Za-z-]+(?=\\s*=)");
    rule.format = m_attributeFormat;
    m_rules.append(rule);

    // Strings
    rule.pattern = QRegularExpression("\"[^\"]*\"|'[^']*'");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    // Comments
    m_commentStartExpression = QRegularExpression("<!--");
    m_commentEndExpression = QRegularExpression("-->");
}

void SyntaxHighlighter::loadCssRules()
{
    HighlightingRule rule;

    // Selectors
    rule.pattern = QRegularExpression("[.#]?[A-Za-z_-][A-Za-z0-9_-]*(?=\\s*[{,])");
    rule.format = m_tagFormat;
    m_rules.append(rule);

    // Properties
    rule.pattern = QRegularExpression("[A-Za-z-]+(?=\\s*:)");
    rule.format = m_attributeFormat;
    m_rules.append(rule);

    // Values
    rule.pattern = QRegularExpression("#[0-9A-Fa-f]{3,8}\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);

    // Numbers
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*(px|em|rem|%|vh|vw|pt)?\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);

    // Strings
    rule.pattern = QRegularExpression("\"[^\"]*\"|'[^']*'");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    // Comments
    m_commentStartExpression = QRegularExpression("/\\*");
    m_commentEndExpression = QRegularExpression("\\*/");
}

void SyntaxHighlighter::loadJsonRules()
{
    HighlightingRule rule;

    // Keys
    rule.pattern = QRegularExpression("\"[^\"]+\"(?=\\s*:)");
    rule.format = m_attributeFormat;
    m_rules.append(rule);

    // Strings
    rule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    // Numbers
    rule.pattern = QRegularExpression("-?\\b[0-9]+\\.?[0-9]*([eE][+-]?[0-9]+)?\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);

    // Booleans and null
    rule.pattern = QRegularExpression("\\b(true|false|null)\\b");
    rule.format = m_keywordFormat;
    m_rules.append(rule);
}

void SyntaxHighlighter::loadYamlRules()
{
    HighlightingRule rule;

    // Keys
    rule.pattern = QRegularExpression("^\\s*[A-Za-z_][A-Za-z0-9_]*(?=\\s*:)");
    rule.format = m_attributeFormat;
    m_rules.append(rule);

    // Strings
    rule.pattern = QRegularExpression("\"[^\"]*\"|'[^']*'");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    // Numbers
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);

    // Booleans
    rule.pattern = QRegularExpression("\\b(true|false|yes|no|on|off|null)\\b", QRegularExpression::CaseInsensitiveOption);
    rule.format = m_keywordFormat;
    m_rules.append(rule);

    // Comments
    rule.pattern = QRegularExpression("#[^\n]*");
    rule.format = m_commentFormat;
    m_rules.append(rule);
}

void SyntaxHighlighter::loadMarkdownRules()
{
    HighlightingRule rule;

    // Headers
    QTextCharFormat headerFormat;
    headerFormat.setForeground(QColor(86, 156, 214));
    headerFormat.setFontWeight(QFont::Bold);

    rule.pattern = QRegularExpression("^#{1,6}\\s+.*$");
    rule.format = headerFormat;
    m_rules.append(rule);

    // Bold
    QTextCharFormat boldFormat;
    boldFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\\*\\*[^*]+\\*\\*|__[^_]+__");
    rule.format = boldFormat;
    m_rules.append(rule);

    // Italic
    QTextCharFormat italicFormat;
    italicFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("\\*[^*]+\\*|_[^_]+_");
    rule.format = italicFormat;
    m_rules.append(rule);

    // Code
    rule.pattern = QRegularExpression("`[^`]+`");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    // Links
    rule.pattern = QRegularExpression("\\[([^\\]]+)\\]\\(([^)]+)\\)");
    rule.format = m_attributeFormat;
    m_rules.append(rule);

    // Lists
    rule.pattern = QRegularExpression("^\\s*[-*+]\\s+");
    rule.format = m_keywordFormat;
    m_rules.append(rule);

    // Numbered lists
    rule.pattern = QRegularExpression("^\\s*[0-9]+\\.\\s+");
    rule.format = m_keywordFormat;
    m_rules.append(rule);
}

void SyntaxHighlighter::loadBashRules()
{
    HighlightingRule rule;

    // Keywords
    QStringList keywordPatterns = {
        "\\bif\\b", "\\bthen\\b", "\\belse\\b", "\\belif\\b", "\\bfi\\b",
        "\\bfor\\b", "\\bwhile\\b", "\\bdo\\b", "\\bdone\\b", "\\bcase\\b",
        "\\besac\\b", "\\bfunction\\b", "\\bin\\b", "\\bselect\\b", "\\buntil\\b",
        "\\breturn\\b", "\\bexit\\b", "\\bbreak\\b", "\\bcontinue\\b",
        "\\bexport\\b", "\\bsource\\b", "\\balias\\b", "\\bunalias\\b",
        "\\blocal\\b", "\\breadonly\\b", "\\bdeclare\\b", "\\btypeset\\b"
    };

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    // Variables
    rule.pattern = QRegularExpression("\\$[A-Za-z_][A-Za-z0-9_]*|\\$\\{[^}]+\\}|\\$[0-9@#?$!*-]");
    rule.format = m_variableFormat;
    m_rules.append(rule);

    // Strings
    rule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"|'[^']*'");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    // Comments
    rule.pattern = QRegularExpression("#[^\n]*");
    rule.format = m_commentFormat;
    m_rules.append(rule);

    // Shebang
    rule.pattern = QRegularExpression("^#![^\n]*");
    rule.format = m_preprocessorFormat;
    m_rules.append(rule);
}

void SyntaxHighlighter::loadRustRules()
{
    HighlightingRule rule;

    // Keywords
    QStringList keywordPatterns = {
        "\\bas\\b", "\\bbreak\\b", "\\bconst\\b", "\\bcontinue\\b", "\\bcrate\\b",
        "\\belse\\b", "\\benum\\b", "\\bextern\\b", "\\bfalse\\b", "\\bfn\\b",
        "\\bfor\\b", "\\bif\\b", "\\bimpl\\b", "\\bin\\b", "\\blet\\b",
        "\\bloop\\b", "\\bmatch\\b", "\\bmod\\b", "\\bmove\\b", "\\bmut\\b",
        "\\bpub\\b", "\\bref\\b", "\\breturn\\b", "\\bself\\b", "\\bSelf\\b",
        "\\bstatic\\b", "\\bstruct\\b", "\\bsuper\\b", "\\btrait\\b", "\\btrue\\b",
        "\\btype\\b", "\\bunsafe\\b", "\\buse\\b", "\\bwhere\\b", "\\bwhile\\b",
        "\\basync\\b", "\\bawait\\b", "\\bdyn\\b"
    };

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    // Types
    QStringList typePatterns = {
        "\\bi8\\b", "\\bi16\\b", "\\bi32\\b", "\\bi64\\b", "\\bi128\\b",
        "\\bu8\\b", "\\bu16\\b", "\\bu32\\b", "\\bu64\\b", "\\bu128\\b",
        "\\bisize\\b", "\\busize\\b", "\\bf32\\b", "\\bf64\\b", "\\bbool\\b",
        "\\bchar\\b", "\\bstr\\b", "\\bString\\b", "\\bVec\\b", "\\bOption\\b",
        "\\bResult\\b", "\\bBox\\b", "\\bRc\\b", "\\bArc\\b"
    };

    for (const QString &pattern : typePatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_typeFormat;
        m_rules.append(rule);
    }

    // Macros
    rule.pattern = QRegularExpression("[A-Za-z_][A-Za-z0-9_]*!");
    rule.format = m_preprocessorFormat;
    m_rules.append(rule);

    // Attributes
    rule.pattern = QRegularExpression("#\\[.*\\]");
    rule.format = m_preprocessorFormat;
    m_rules.append(rule);

    // Functions
    rule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\s*\\()");
    rule.format = m_functionFormat;
    m_rules.append(rule);

    // Numbers
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*(_[0-9]+)*([eE][+-]?[0-9]+)?[if]?(8|16|32|64|128|size)?\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);

    // Strings
    rule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    // Character literals
    rule.pattern = QRegularExpression("'[^'\\\\]*(\\\\.[^'\\\\]*)*'");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    // Comments
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = m_commentFormat;
    m_rules.append(rule);

    m_commentStartExpression = QRegularExpression("/\\*");
    m_commentEndExpression = QRegularExpression("\\*/");
}

void SyntaxHighlighter::loadGoRules()
{
    HighlightingRule rule;

    // Keywords
    QStringList keywordPatterns = {
        "\\bbreak\\b", "\\bcase\\b", "\\bchan\\b", "\\bconst\\b", "\\bcontinue\\b",
        "\\bdefault\\b", "\\bdefer\\b", "\\belse\\b", "\\bfallthrough\\b", "\\bfor\\b",
        "\\bfunc\\b", "\\bgo\\b", "\\bgoto\\b", "\\bif\\b", "\\bimport\\b",
        "\\binterface\\b", "\\bmap\\b", "\\bpackage\\b", "\\brange\\b", "\\breturn\\b",
        "\\bselect\\b", "\\bstruct\\b", "\\bswitch\\b", "\\btype\\b", "\\bvar\\b",
        "\\btrue\\b", "\\bfalse\\b", "\\bnil\\b"
    };

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    // Types
    QStringList typePatterns = {
        "\\bbool\\b", "\\bbyte\\b", "\\bcomplex64\\b", "\\bcomplex128\\b",
        "\\berror\\b", "\\bfloat32\\b", "\\bfloat64\\b", "\\bint\\b",
        "\\bint8\\b", "\\bint16\\b", "\\bint32\\b", "\\bint64\\b",
        "\\brune\\b", "\\bstring\\b", "\\buint\\b", "\\buint8\\b",
        "\\buint16\\b", "\\buint32\\b", "\\buint64\\b", "\\buintptr\\b"
    };

    for (const QString &pattern : typePatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_typeFormat;
        m_rules.append(rule);
    }

    // Functions
    rule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\s*\\()");
    rule.format = m_functionFormat;
    m_rules.append(rule);

    // Numbers
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*([eE][+-]?[0-9]+)?i?\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);

    // Strings
    rule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"|`[^`]*`");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    // Comments
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = m_commentFormat;
    m_rules.append(rule);

    m_commentStartExpression = QRegularExpression("/\\*");
    m_commentEndExpression = QRegularExpression("\\*/");
}

void SyntaxHighlighter::loadJavaRules()
{
    HighlightingRule rule;

    // Keywords
    QStringList keywordPatterns = {
        "\\babstract\\b", "\\bassert\\b", "\\bboolean\\b", "\\bbreak\\b", "\\bbyte\\b",
        "\\bcase\\b", "\\bcatch\\b", "\\bchar\\b", "\\bclass\\b", "\\bconst\\b",
        "\\bcontinue\\b", "\\bdefault\\b", "\\bdo\\b", "\\bdouble\\b", "\\belse\\b",
        "\\benum\\b", "\\bextends\\b", "\\bfinal\\b", "\\bfinally\\b", "\\bfloat\\b",
        "\\bfor\\b", "\\bgoto\\b", "\\bif\\b", "\\bimplements\\b", "\\bimport\\b",
        "\\binstanceof\\b", "\\bint\\b", "\\binterface\\b", "\\blong\\b", "\\bnative\\b",
        "\\bnew\\b", "\\bpackage\\b", "\\bprivate\\b", "\\bprotected\\b", "\\bpublic\\b",
        "\\breturn\\b", "\\bshort\\b", "\\bstatic\\b", "\\bstrictfp\\b", "\\bsuper\\b",
        "\\bswitch\\b", "\\bsynchronized\\b", "\\bthis\\b", "\\bthrow\\b", "\\bthrows\\b",
        "\\btransient\\b", "\\btry\\b", "\\bvoid\\b", "\\bvolatile\\b", "\\bwhile\\b",
        "\\btrue\\b", "\\bfalse\\b", "\\bnull\\b", "\\bvar\\b", "\\brecord\\b",
        "\\bsealed\\b", "\\bpermits\\b", "\\byield\\b"
    };

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    // Annotations
    rule.pattern = QRegularExpression("@[A-Za-z_][A-Za-z0-9_]*");
    rule.format = m_preprocessorFormat;
    m_rules.append(rule);

    // Functions
    rule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\s*\\()");
    rule.format = m_functionFormat;
    m_rules.append(rule);

    // Numbers
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*[fFdDlL]?\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);

    // Strings
    rule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    // Character literals
    rule.pattern = QRegularExpression("'[^'\\\\]*(\\\\.[^'\\\\]*)*'");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    // Comments
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = m_commentFormat;
    m_rules.append(rule);

    m_commentStartExpression = QRegularExpression("/\\*");
    m_commentEndExpression = QRegularExpression("\\*/");
}

void SyntaxHighlighter::loadSqlRules()
{
    HighlightingRule rule;

    // Keywords
    QStringList keywordPatterns = {
        "\\bSELECT\\b", "\\bFROM\\b", "\\bWHERE\\b", "\\bAND\\b", "\\bOR\\b",
        "\\bNOT\\b", "\\bIN\\b", "\\bIS\\b", "\\bNULL\\b", "\\bLIKE\\b",
        "\\bBETWEEN\\b", "\\bJOIN\\b", "\\bINNER\\b", "\\bLEFT\\b", "\\bRIGHT\\b",
        "\\bOUTER\\b", "\\bON\\b", "\\bAS\\b", "\\bORDER\\b", "\\bBY\\b",
        "\\bASC\\b", "\\bDESC\\b", "\\bGROUP\\b", "\\bHAVING\\b", "\\bLIMIT\\b",
        "\\bOFFSET\\b", "\\bINSERT\\b", "\\bINTO\\b", "\\bVALUES\\b", "\\bUPDATE\\b",
        "\\bSET\\b", "\\bDELETE\\b", "\\bCREATE\\b", "\\bALTER\\b", "\\bDROP\\b",
        "\\bTABLE\\b", "\\bINDEX\\b", "\\bVIEW\\b", "\\bDATABASE\\b", "\\bPRIMARY\\b",
        "\\bKEY\\b", "\\bFOREIGN\\b", "\\bREFERENCES\\b", "\\bUNIQUE\\b", "\\bCHECK\\b",
        "\\bDEFAULT\\b", "\\bAUTO_INCREMENT\\b", "\\bCASCADE\\b", "\\bCONSTRAINT\\b",
        "\\bEXISTS\\b", "\\bDISTINCT\\b", "\\bUNION\\b", "\\bALL\\b", "\\bCASE\\b",
        "\\bWHEN\\b", "\\bTHEN\\b", "\\bELSE\\b", "\\bEND\\b", "\\bCOUNT\\b",
        "\\bSUM\\b", "\\bAVG\\b", "\\bMIN\\b", "\\bMAX\\b", "\\bTRUE\\b", "\\bFALSE\\b"
    };

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    // Types
    QStringList typePatterns = {
        "\\bINT\\b", "\\bINTEGER\\b", "\\bSMALLINT\\b", "\\bBIGINT\\b",
        "\\bFLOAT\\b", "\\bDOUBLE\\b", "\\bDECIMAL\\b", "\\bNUMERIC\\b",
        "\\bCHAR\\b", "\\bVARCHAR\\b", "\\bTEXT\\b", "\\bBLOB\\b",
        "\\bDATE\\b", "\\bTIME\\b", "\\bDATETIME\\b", "\\bTIMESTAMP\\b",
        "\\bBOOLEAN\\b", "\\bBOOL\\b"
    };

    for (const QString &pattern : typePatterns) {
        rule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
        rule.format = m_typeFormat;
        m_rules.append(rule);
    }

    // Numbers
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);

    // Strings
    rule.pattern = QRegularExpression("'[^']*'");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    // Comments
    rule.pattern = QRegularExpression("--[^\n]*");
    rule.format = m_commentFormat;
    m_rules.append(rule);

    m_commentStartExpression = QRegularExpression("/\\*");
    m_commentEndExpression = QRegularExpression("\\*/");
}

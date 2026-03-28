#include "highlighter.h"
#include "ui/theme.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    setupFormats();
}

void SyntaxHighlighter::setupFormats()
{
    Theme theme = ThemeManager::instance().currentTheme();

    m_keywordFormat.setForeground(theme.keyword);
    m_keywordFormat.setFontWeight(QFont::Bold);

    m_typeFormat.setForeground(theme.type);
    m_functionFormat.setForeground(theme.function);
    m_stringFormat.setForeground(theme.string);
    m_numberFormat.setForeground(theme.number);

    m_commentFormat.setForeground(theme.comment);
    m_commentFormat.setFontItalic(true);

    m_preprocessorFormat.setForeground(theme.variable);
    m_operatorFormat.setForeground(theme.operator_);
    m_tagFormat.setForeground(theme.keyword);
    m_attributeFormat.setForeground(theme.variable);
    m_variableFormat.setForeground(theme.variable);
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
            QRegularExpressionMatch endMatch =
                m_commentEndExpression.match(text, startIndex + 2);
            int endIndex = endMatch.capturedStart();
            int commentLength;

            if (endIndex == -1) {
                setCurrentBlockState(1);
                commentLength = text.length() - startIndex;
            } else {
                commentLength =
                    endIndex - startIndex + endMatch.capturedLength();
            }

            setFormat(startIndex, commentLength, m_commentFormat);
            startIndex = text.indexOf(
                m_commentStartExpression, startIndex + commentLength);
        }
    }
}

void SyntaxHighlighter::loadLanguageRules(const QString &language)
{
    QString lang = language.toLower();

    // Try exact name match first
    QString path = ":/syntax/" + lang + ".json";
    QFile file(path);
    if (!file.exists()) {
        // Try common aliases
        static const QMap<QString, QString> aliases = {
            {"c++", "cpp"}, {"c", "c"}, {"py", "python"},
            {"js", "javascript"}, {"ts", "typescript"},
            {"htm", "html"}, {"yml", "yaml"}, {"md", "markdown"},
            {"sh", "bash"}, {"shell", "bash"}, {"rs", "rust"},
            {"cs", "csharp"}, {"c#", "csharp"},
            {"rb", "ruby"}, {"pl", "perl"}, {"ps1", "powershell"},
            {"f#", "fsharp"}, {"vb", "visualbasic"},
            {"visual basic", "visualbasic"},
            {"objective-c", "objectivec"},
            {"objective-c++", "objectivecpp"},
            {"asm", "assembly"}, {"s", "assembly"},
            {"kt", "kotlin"}, {"kts", "kotlin"},
            {"ex", "elixir"}, {"exs", "elixir"},
            {"erl", "erlang"}, {"hs", "haskell"},
            {"ml", "ocaml"}, {"mli", "ocaml"},
            {"jl", "julia"}, {"m", "matlab"},
            {"pas", "pascal"}, {"adb", "ada"}, {"ads", "ada"},
            {"cob", "cobol"}, {"cbl", "cobol"},
            {"v", "verilog"}, {"vhd", "vhdl"}, {"vhdl", "vhdl"},
            {"scm", "scheme"}, {"rkt", "scheme"},
            {"cl", "lisp"}, {"lisp", "lisp"},
            {"rst", "restructuredtext"},
            {"tex", "latex"}, {"sty", "latex"},
            {"sass", "sass"}, {"dockerfile", "dockerfile"},
            {"cmake", "cmake"}, {"makefile", "makefile"},
            {"pgsql", "postgresql"},
            {"tf", "terraform"}, {"tfvars", "terraform"},
            {"sol", "solidity"}, {"wat", "wasm"},
            {"vert", "glsl"}, {"frag", "glsl"}, {"glsl", "glsl"},
            {"cu", "cuda"}, {"cuh", "cuda"},
            {"coffee", "coffeescript"},
            {"cr", "crystal"}, {"vlang", "v"},
            {"gql", "graphql"}, {"proto", "protobuf"},
            {"protocol buffers", "protobuf"},
            {"webassembly", "wasm"},
            {"plain text", "plaintext"},
        };
        QString alias = aliases.value(lang, lang);
        path = ":/syntax/" + alias + ".json";
    }
    loadFromDefinition(path);
}

void SyntaxHighlighter::loadFromDefinition(const QString &resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return;
    }
    QJsonObject obj = doc.object();

    HighlightingRule rule;

    // Check for case-insensitive keywords
    bool caseInsensitive = obj["caseInsensitiveKeywords"].toBool(false);
    QRegularExpression::PatternOptions opts =
        caseInsensitive ? QRegularExpression::CaseInsensitiveOption
                        : QRegularExpression::NoPatternOption;

    // Keywords
    QJsonArray keywords = obj["keywords"].toArray();
    for (const QJsonValue &kw : keywords) {
        rule.pattern = QRegularExpression(
            "\\b" + QRegularExpression::escape(kw.toString()) + "\\b", opts);
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    // Types
    QJsonArray types = obj["types"].toArray();
    for (const QJsonValue &t : types) {
        rule.pattern = QRegularExpression(
            "\\b" + QRegularExpression::escape(t.toString()) + "\\b", opts);
        rule.format = m_typeFormat;
        m_rules.append(rule);
    }

    // Builtins (use type format)
    QJsonArray builtins = obj["builtins"].toArray();
    for (const QJsonValue &b : builtins) {
        rule.pattern = QRegularExpression(
            "\\b" + QRegularExpression::escape(b.toString()) + "\\b");
        rule.format = m_typeFormat;
        m_rules.append(rule);
    }

    // Numbers
    QString numPattern = obj["numberPattern"].toString();
    if (!numPattern.isEmpty()) {
        rule.pattern = QRegularExpression(numPattern);
        rule.format = m_numberFormat;
        m_rules.append(rule);
    }

    // Strings
    QJsonArray stringDelims = obj["stringDelimiters"].toArray();
    for (const QJsonValue &d : stringDelims) {
        QString delim = QRegularExpression::escape(d.toString());
        rule.pattern = QRegularExpression(
            delim + "(?:[^" + delim + "\\\\\\n]|\\\\.)*" + delim);
        rule.format = m_stringFormat;
        m_rules.append(rule);
    }

    // Tag patterns (HTML/XML)
    QString tagPat = obj["tagPattern"].toString();
    if (!tagPat.isEmpty()) {
        rule.pattern = QRegularExpression(tagPat);
        rule.format = m_tagFormat;
        m_rules.append(rule);

        // Also add closing bracket
        rule.pattern = QRegularExpression("/?>");
        rule.format = m_tagFormat;
        m_rules.append(rule);
    }

    // Attribute patterns (HTML/XML)
    QString attrPat = obj["attributePattern"].toString();
    if (!attrPat.isEmpty()) {
        rule.pattern = QRegularExpression(attrPat);
        rule.format = m_attributeFormat;
        m_rules.append(rule);
    }

    // Selector patterns (CSS)
    QString selPat = obj["selectorPattern"].toString();
    if (!selPat.isEmpty()) {
        rule.pattern = QRegularExpression(selPat);
        rule.format = m_tagFormat;
        m_rules.append(rule);
    }

    // Property patterns (CSS)
    QString propPat = obj["propertyPattern"].toString();
    if (!propPat.isEmpty()) {
        rule.pattern = QRegularExpression(propPat);
        rule.format = m_attributeFormat;
        m_rules.append(rule);
    }

    // Key patterns (JSON/YAML/TOML/INI)
    QString keyPat = obj["keyPattern"].toString();
    if (!keyPat.isEmpty()) {
        rule.pattern = QRegularExpression(keyPat);
        rule.format = m_attributeFormat;
        m_rules.append(rule);
    }

    // Section patterns (TOML/INI)
    QString sectionPat = obj["sectionPattern"].toString();
    if (!sectionPat.isEmpty()) {
        rule.pattern = QRegularExpression(sectionPat);
        rule.format = m_tagFormat;
        m_rules.append(rule);
    }

    // Variable patterns (Bash/PHP/PowerShell/etc.)
    QString varPat = obj["variablePattern"].toString();
    if (!varPat.isEmpty()) {
        rule.pattern = QRegularExpression(varPat);
        rule.format = m_variableFormat;
        m_rules.append(rule);
    }

    // Macro patterns (Rust)
    QString macroPat = obj["macroPattern"].toString();
    if (!macroPat.isEmpty()) {
        rule.pattern = QRegularExpression(macroPat);
        rule.format = m_preprocessorFormat;
        m_rules.append(rule);
    }

    // Command patterns (LaTeX)
    QString cmdPat = obj["commandPattern"].toString();
    if (!cmdPat.isEmpty()) {
        rule.pattern = QRegularExpression(cmdPat);
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    // Directive patterns (reStructuredText)
    QString dirPat = obj["directivePattern"].toString();
    if (!dirPat.isEmpty()) {
        rule.pattern = QRegularExpression(dirPat);
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    // Role patterns (reStructuredText)
    QString rolePat = obj["rolePattern"].toString();
    if (!rolePat.isEmpty()) {
        rule.pattern = QRegularExpression(rolePat);
        rule.format = m_typeFormat;
        m_rules.append(rule);
    }

    // Header patterns (Markdown/reStructuredText)
    QString headerPat = obj["headerPattern"].toString();
    if (!headerPat.isEmpty()) {
        QTextCharFormat headerFormat;
        headerFormat.setForeground(m_keywordFormat.foreground());
        headerFormat.setFontWeight(QFont::Bold);
        rule.pattern = QRegularExpression(headerPat);
        rule.format = headerFormat;
        m_rules.append(rule);
    }

    // Bold patterns (Markdown)
    QString boldPat = obj["boldPattern"].toString();
    if (!boldPat.isEmpty()) {
        QTextCharFormat boldFormat;
        boldFormat.setFontWeight(QFont::Bold);
        rule.pattern = QRegularExpression(boldPat);
        rule.format = boldFormat;
        m_rules.append(rule);
    }

    // Italic patterns (Markdown)
    QString italicPat = obj["italicPattern"].toString();
    if (!italicPat.isEmpty()) {
        QTextCharFormat italicFormat;
        italicFormat.setFontItalic(true);
        rule.pattern = QRegularExpression(italicPat);
        rule.format = italicFormat;
        m_rules.append(rule);
    }

    // Code patterns (Markdown)
    QString codePat = obj["codePattern"].toString();
    if (!codePat.isEmpty()) {
        rule.pattern = QRegularExpression(codePat);
        rule.format = m_stringFormat;
        m_rules.append(rule);
    }

    // Link patterns (Markdown)
    QString linkPat = obj["linkPattern"].toString();
    if (!linkPat.isEmpty()) {
        rule.pattern = QRegularExpression(linkPat);
        rule.format = m_attributeFormat;
        m_rules.append(rule);
    }

    // List patterns (Markdown)
    QString listPat = obj["listPattern"].toString();
    if (!listPat.isEmpty()) {
        rule.pattern = QRegularExpression(listPat);
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    // Shebang patterns (Bash)
    QString shebangPat = obj["shebangPattern"].toString();
    if (!shebangPat.isEmpty()) {
        rule.pattern = QRegularExpression(shebangPat);
        rule.format = m_preprocessorFormat;
        m_rules.append(rule);
    }

    // Function calls
    QString funcPattern = obj["functionPattern"].toString();
    if (!funcPattern.isEmpty()) {
        rule.pattern = QRegularExpression(funcPattern);
        rule.format = m_functionFormat;
        m_rules.append(rule);
    }

    // Single-line comments
    QString slComment = obj["singleLineComment"].toString();
    if (!slComment.isEmpty()) {
        rule.pattern = QRegularExpression(
            QRegularExpression::escape(slComment) + "[^\n]*");
        rule.format = m_commentFormat;
        m_rules.append(rule);
    }

    // Preprocessor
    QString ppPrefix = obj["preprocessorPrefix"].toString();
    if (!ppPrefix.isEmpty()) {
        rule.pattern = QRegularExpression(
            "^\\s*" + QRegularExpression::escape(ppPrefix) + "[^\n]*");
        rule.format = m_preprocessorFormat;
        m_rules.append(rule);
    }

    // Custom rules (for languages like Diff that need arbitrary pattern→format mappings)
    QJsonArray customRules = obj["customRules"].toArray();
    for (const QJsonValue &cr : customRules) {
        QJsonObject crObj = cr.toObject();
        QString pat = crObj["pattern"].toString();
        QString fmt = crObj["format"].toString();
        if (pat.isEmpty()) continue;

        rule.pattern = QRegularExpression(pat);
        if (fmt == "keyword") rule.format = m_keywordFormat;
        else if (fmt == "type") rule.format = m_typeFormat;
        else if (fmt == "function") rule.format = m_functionFormat;
        else if (fmt == "string") rule.format = m_stringFormat;
        else if (fmt == "number") rule.format = m_numberFormat;
        else if (fmt == "comment") rule.format = m_commentFormat;
        else if (fmt == "preprocessor") rule.format = m_preprocessorFormat;
        else if (fmt == "operator") rule.format = m_operatorFormat;
        else if (fmt == "tag") rule.format = m_tagFormat;
        else if (fmt == "attribute") rule.format = m_attributeFormat;
        else if (fmt == "variable") rule.format = m_variableFormat;
        else continue;

        m_rules.append(rule);
    }

    // Multi-line comments
    QString mlStart = obj["multiLineCommentStart"].toString();
    QString mlEnd = obj["multiLineCommentEnd"].toString();
    if (!mlStart.isEmpty() && !mlEnd.isEmpty()) {
        m_commentStartExpression =
            QRegularExpression(QRegularExpression::escape(mlStart));
        m_commentEndExpression =
            QRegularExpression(QRegularExpression::escape(mlEnd));
    } else {
        m_commentStartExpression = QRegularExpression();
        m_commentEndExpression = QRegularExpression();
    }
}

#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QTextDocument *parent = nullptr);

    void setLanguage(const QString &language);
    QString language() const { return m_language; }
    void setupFormats();

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    void loadLanguageRules(const QString &language);
    void loadFromDefinition(const QString &resourcePath);

    QString m_language;
    QVector<HighlightingRule> m_rules;

    // Formats
    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_typeFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_preprocessorFormat;
    QTextCharFormat m_operatorFormat;
    QTextCharFormat m_tagFormat;
    QTextCharFormat m_attributeFormat;
    QTextCharFormat m_variableFormat;

    // Multi-line state
    QRegularExpression m_commentStartExpression;
    QRegularExpression m_commentEndExpression;
    QRegularExpression m_stringStartExpression;
    QRegularExpression m_stringEndExpression;
};

#endif // HIGHLIGHTER_H
